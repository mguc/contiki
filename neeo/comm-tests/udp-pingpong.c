/*
 * Copyright (c) 2014, Thingsquare, www.thingsquare.com.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#if RIMESTATS_CONF_ENABLED
#include "net/rime/rimestats.h"
#endif /* RIMESTATS_CONF_ENABLED */

#include "udp-socket.h"
#include "uip-debug.h"

#include "dev/leds.h"
#include "node-id.h"

#include <stdio.h>
#include <string.h>

#define DEBUG 0

#if DEBUG
#define PRINTF(...)           printf(__VA_ARGS__)
#define COOJA_DEBUG(x)        cooja_debug(x)
#else
#define PRINTF(...)
#define COOJA_DEBUG(x)
#endif  /* debug */

/* If pingpong ends, how long should we wait until we start up a new one? */
#define TIMEOUT (5*CLOCK_SECOND)
#define TIMEOUT_RANDOM (5*CLOCK_SECOND)
#define BROADCAST_PERIOD (3*CLOCK_SECOND)

static struct timer timeout_timer;

static uip_ipaddr_t neighbor;
static uip_ipaddr_t unspec;
static uip_ipaddr_t multicast_addr;

#define PORT_UDP 200
static struct udp_socket s;

#define BROADCAST_SIZE 150
#define UNICAST_MIN_SIZE 10
#define UNICAST_MAX_SIZE 150
static int unicast_current_size = UNICAST_MIN_SIZE;

/* stats */
#define TEST_PERIOD_SECS 30

static int sent;
static int received;

static int timeouts = 0;

/*---------------------------------------------------------------------------*/
PROCESS(pingpong_process, "Ping pong process");
PROCESS(print_stats_process, "Print stats");
AUTOSTART_PROCESSES(&pingpong_process, &print_stats_process);
/*---------------------------------------------------------------------------*/
static void
send_pingpong(void)
{
  clock_time_t timeout = TIMEOUT
      + (clock_time_t)(random_rand() % (TIMEOUT_RANDOM));
  timer_set(&timeout_timer, timeout);

  if(uip_ipaddr_cmp(&neighbor, &unspec)) {
    PRINTF("unspec neighbor\n");
    return;
  }

  sent++;

  char* buf = "unicast";
  PRINTF("sending unicast\n");
  udp_socket_sendto(&s, buf, unicast_current_size, &neighbor, PORT_UDP);
}
/*---------------------------------------------------------------------------*/
static void
udp_receiver(struct udp_socket *c,
             void *ptr,
             const uip_ipaddr_t *sender_addr,
             uint16_t sender_port,
             const uip_ipaddr_t *receiver_addr,
             uint16_t receiver_port,
             const uint8_t *data,
             uint16_t datalen)
{
  if(!uip_ipaddr_cmp(&neighbor, sender_addr)) {
    printf("new pingpong remote: ");
    uip_debug_ipaddr_print(sender_addr);
//    printf(", I am: ");
//    uip_debug_ipaddr_print(rpl_dag_root_get_local_address());
    printf("\n");
    uip_ipaddr_copy(&neighbor, sender_addr);
  }

  /* Register pingpong remote */
  if(uip_ipaddr_cmp(&multicast_addr, receiver_addr)) {
    PRINTF("got multicast\n");
    return;
  }

  /* Continue pingpong */
  PRINTF("got pingpong\n");
  received++;
  send_pingpong();
}
/*---------------------------------------------------------------------------*/
static void
keepalive_pingpongs(void* ptr)
{
  /* This is called periodically. Check if we haven't receiving any pingpongs
   * lately. */
  ctimer_set(ptr, CLOCK_SECOND, keepalive_pingpongs, ptr);
  if(uip_ipaddr_cmp(&neighbor, &unspec)) {
    return;
  }
  if(!timer_expired(&timeout_timer)) {
    /* we're still receiving pingpongs */
    return;
  }

  /* Restart pingpong */
  PRINTF("restarting pingpong\n");
  timeouts++;
  send_pingpong();
}
/*---------------------------------------------------------------------------*/
static void
send_broadcast(void* ptr)
{
  char* buf = "broadcast";

  printf("sending broadcast from: ");
//  uip_debug_ipaddr_print(rpl_dag_root_get_local_address());
  printf("\n");
  udp_socket_sendto(&s, buf, BROADCAST_SIZE, &multicast_addr, PORT_UDP);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(pingpong_process, ev, data)
{
  static struct ctimer broadcast_ctimer;
  static struct ctimer keepalive_ctimer;
  static struct etimer et;

  PROCESS_BEGIN();

  sent = 0;
  received = 0;

  uip_create_unspecified(&unspec);
  uip_create_unspecified(&neighbor);
  uip_create_linklocal_allnodes_mcast(&multicast_addr);

  udp_socket_register(&s, NULL, udp_receiver);
  udp_socket_bind(&s, PORT_UDP);
  udp_socket_connect(&s, NULL, PORT_UDP);

  clock_time_t timeout = TIMEOUT
      + (clock_time_t)(random_rand() % (TIMEOUT_RANDOM));
  timer_set(&timeout_timer, timeout);
  ctimer_set(&keepalive_ctimer, CLOCK_SECOND, keepalive_pingpongs,
             (void*)&keepalive_ctimer); /* reschedules itself */

  while(1) {
    /* We periodically and randomly send broadcast messages, identifying us to
     * neighboring devices, and also disturbing ongoing pingpong transmissions. */
    etimer_set(&et, BROADCAST_PERIOD);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    clock_time_t delay = (clock_time_t)(random_rand() % (BROADCAST_PERIOD));
    ctimer_set(&broadcast_ctimer, delay, send_broadcast, NULL);

    /* XXX We only broadcast once! */
    PROCESS_YIELD_UNTIL(0);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(print_stats_process, ev, data)
{
  static struct etimer periodic_timer;
  static uint32_t cpu_percentage;
  static uint32_t rf_percentage;

  PROCESS_BEGIN();

  while(1) {
    etimer_set(&periodic_timer, TEST_PERIOD_SECS * CLOCK_SECOND);
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));

    static uint32_t last_cpu, last_lpm, last_transmit, last_listen;

    uint32_t cpu, lpm, transmit, listen;
    uint32_t all_cpu, all_lpm, all_transmit, all_listen;
    uint32_t radio, time;

    energest_flush();

    all_cpu = energest_type_time(ENERGEST_TYPE_CPU);
    all_lpm = energest_type_time(ENERGEST_TYPE_LPM);
    all_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
    all_listen = energest_type_time(ENERGEST_TYPE_LISTEN);

    cpu = all_cpu - last_cpu;
    lpm = all_lpm - last_lpm;
    transmit = all_transmit - last_transmit;
    listen = all_listen - last_listen;

    last_cpu = energest_type_time(ENERGEST_TYPE_CPU);
    last_lpm = energest_type_time(ENERGEST_TYPE_LPM);
    last_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
    last_listen = energest_type_time(ENERGEST_TYPE_LISTEN);

    radio = transmit + listen;
    time = cpu + lpm;
    if(time == 0) {
      time = 1;
    }

    cpu_percentage = (cpu * 1000) / time;
    rf_percentage = (radio * 1000) / time;

    if(cpu_percentage > 1000) {
      cpu_percentage = 1000;
    }
    if(rf_percentage > 1000) {
      rf_percentage = 1000;
    }

    printf("-------------\n");

    /* Print */
    printf("RF percentage %d.%d\n",
           (int)(rf_percentage / 10),
           (int)(rf_percentage % 10));
    printf("CPU percentage %d.%d\n",
           (int)(cpu_percentage / 10),
           (int)(cpu_percentage % 10));

    /* stats */
    printf("Total number of timeouts until now: %d\n", timeouts);
    printf("Unicast payload size was: %d\n", unicast_current_size);
    int rate = (10UL * (sent + received)) / (TEST_PERIOD_SECS);
    printf("Throughput: sent+received %d+%d last %d seconds: avr %d.%d pkts/sec\n", sent,
           received, TEST_PERIOD_SECS, rate / 10, rate % 10);
    uint32_t efficiency = 10000UL * (8UL * unicast_current_size * (sent + received))
          / (rf_percentage * TEST_PERIOD_SECS); /* NB: rf_percentage is in promilles */
    printf("Power-efficiency: %d.%d bits/radio on second\n", (int)(efficiency/10), (int)(efficiency%10));
#if RIMESTATS_CONF_ENABLED
    printf("Reliability: %ld/%ld %d%% acks received\n", rimestats.ackrx, rimestats.reliabletx, (int) ((100*rimestats.ackrx)/rimestats.reliabletx));
#endif /* RIMESTATS_CONF_ENABLED */
    printf("\n");

    sent = 0;
    received = 0;
#if RIMESTATS_CONF_ENABLED
    rimestats.ackrx = 0;
    rimestats.reliabletx = 0;
#endif /* RIMESTATS_CONF_ENABLED */

    unicast_current_size += 10;
    if(unicast_current_size > UNICAST_MAX_SIZE) {
      unicast_current_size = UNICAST_MIN_SIZE;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
