/*
 * Copyright (c) 2012, Thingsquare, www.thingsquare.com.
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
#include "dev/leds.h"

#include "net/ip/simple-udp.h"
#include "node-id.h"

#include "replong.h"

#include "net/rime/rimestats.h"

#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
#define UDP_PORT 124
#define SEND_INTERVAL (CLOCK_SECOND / 2)

#ifdef UDP_UNICAST_DESTINATION_IP
#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)
#define DESTINATION_IP QUOTEME(UDP_UNICAST_DESTINATION_IP)
#endif /* UDP_UNICAST_DESTINATION_IP */

/* Configuration */
#define MIN_SIZE 1
#define MAX_SIZE 110
#define PRINT_BLOCK_SIZE 10

#define NR_LOGS (MAX_SIZE - MIN_SIZE + 1)

/*---------------------------------------------------------------------------*/
static uint8_t
get_rf_channel(void)
{
  radio_value_t chan;

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);

  return (uint8_t)chan;
}
/*---------------------------------------------------------------------------*/
static struct simple_udp_connection unicast_connection;
/*---------------------------------------------------------------------------*/
PROCESS(udp_unicast_process, "Test UDP unicast process");
PROCESS(print_rfusage_process, "Print RF energest");
AUTOSTART_PROCESSES(&udp_unicast_process, &print_rfusage_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr, uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
         const uint8_t *data, uint16_t datalen)
{
  /*printf("RX port %d from port %d with length %d\n", receiver_port,
     sender_port, datalen);*/
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_unicast_process, ev, data)
{
  static struct etimer periodic_timer;
  static uip_ipaddr_t addr;

  PROCESS_BEGIN();

  printf("Starting UDP unicast sender.\n");
  /* Note: set destination to NULL, to allow receiving packets from anyone */
  simple_udp_register(&unicast_connection, UDP_PORT, NULL, UDP_PORT,
                      receiver);

  replong_init();

#ifdef DESTINATION_IP
  uiplib_ip6addrconv(DESTINATION_IP, &addr);
#else /* DESTINATION_IP */

  const struct replong_neighbor *n;

  do {
    /* Use replong to find a random neighbor to send to. */
#define NEIGHBOR_COLLECTION_TIME 10
    replong_send_whothere(NEIGHBOR_COLLECTION_TIME);
    etimer_set(&periodic_timer, CLOCK_SECOND * NEIGHBOR_COLLECTION_TIME);
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));

    /* Check that we have received at least one neighbor. If so, we
       pick the first one. If no neighbors were found, we repeat the
       replong. */
    n = replong_neighbors();
  } while(n == NULL);
  printf("Found a destination: ");
  uip_debug_ipaddr_print(&n->ipaddr);
  printf("\n");
  uip_ipaddr_copy(&addr, &n->ipaddr);

#endif /* DESTINATION_IP */

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    static char buf[MAX_SIZE + 1];
    static int current_size = MIN_SIZE;

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_set(&periodic_timer, SEND_INTERVAL + (random_rand() % CLOCK_SECOND));


    int i;

    current_size++;
    if(current_size > MAX_SIZE) {
      current_size = MIN_SIZE;
    }

    for(i = 0; i < current_size; i++) {
      buf[i] = 'a';
    }
    buf[current_size] = '\0';

#if RIMESTATS_CONF_ON
    if(rimestats.reliabletx != 0) {
      printf("ACKS: %ld/%ld %d%%\n", rimestats.ackrx, rimestats.reliabletx, (int) ((100*rimestats.ackrx)/rimestats.reliabletx));
    }
#endif
    printf("TX[%02d]: '%s'\n", current_size, buf);

      /*{
        int i;
        printf("Sending unicast to: ");
        for(i = 0; i < 7; ++i) {
          printf("%02x%02x:", addr.u8[i * 2], addr.u8[i * 2 + 1]);
        }
        printf("%02x%02x\n", addr.u8[7 * 2], addr.u8[7 * 2 + 1]);
      }*/

    leds_toggle(LEDS_GREEN);
    simple_udp_sendto(&unicast_connection, buf, current_size, &addr);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(print_rfusage_process, ev, data)
{
  static struct etimer periodic_timer;
  static uint32_t cpu_percentage;
  static uint32_t rf_percentage;

  PROCESS_BEGIN();

  while(1) {
    etimer_set(&periodic_timer, 1 * CLOCK_SECOND);
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

    cpu_percentage = (cpu * 100UL) / time;
    rf_percentage = (radio * 100UL) / time;

    if(cpu_percentage > 100) {
      cpu_percentage = 100;
    }
    if(rf_percentage > 100) {
      rf_percentage = 100;
    }

    /* Print */
    printf("RF percentage %d\n", (int)rf_percentage);
    printf("RF channel %d\n", get_rf_channel());

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
