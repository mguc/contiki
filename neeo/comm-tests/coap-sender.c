#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/leds.h"

#include "net/ip/simple-udp.h"
#include "node-id.h"

#include "net/rime/rimestats.h"

#include "er-coap-engine.h"
#include "rest-engine.h"

#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
#define SEND_INTERVAL (5 * CLOCK_SECOND)
#define PORT_BROADCAST 200
#define LEN_MIN 10
#define LEN_MAX 100
#define COAP_PORT SERVER_LISTEN_PORT
#define REALLY_LONG_URI "/update/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t remote;
static uip_ipaddr_t unspec;
static uip_ipaddr_t multicast_addr;
static struct udp_socket broadcast;
static struct ctimer broadcast_ctimer;
/*---------------------------------------------------------------------------*/
static uint8_t
get_rf_channel(void)
{
  radio_value_t chan;

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);

  return (uint8_t)chan;
}
/*---------------------------------------------------------------------------*/
PROCESS(coap_tester_process, "Test Coap sender process");
PROCESS(print_rfusage_process, "Print RF energest");
AUTOSTART_PROCESSES(&coap_tester_process, &print_rfusage_process);
/*---------------------------------------------------------------------------*/
static void
send_coap_message(int confirm, int uri_len)
{
  static coap_packet_t request[1];
  static char* uri[LEN_MAX];

  if(uip_ipaddr_cmp(&remote, &unspec)) {
    printf("error: unknown remote\n");
    return;
  }
  if(confirm) {
    printf("error: confirmable not supported\n");
    return;
  }

  strncpy(uri, REALLY_LONG_URI, uri_len);
  coap_init_message(request, COAP_TYPE_NON, COAP_GET, 0);
  coap_set_header_uri_path(request, uri);
  coap_set_header_block2(request, 0, 0, REST_MAX_CHUNK_SIZE);
  request->mid = coap_get_mid();
  coap_transaction_t *trans = coap_new_transaction(request->mid, &remote,
                                                   COAP_PORT);
  if(trans) {
    trans->packet_len = coap_serialize_message(request, trans->packet);
    coap_send_transaction(trans);
  } else {
    printf("error: could not create new transaction\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
send_broadcast(void* ptr)
{
  static int nr_broadcasts = 0;

  char* buf = "yellow";
  printf("sending broadcast %d/%d\n", nr_broadcasts, 10);
  udp_socket_sendto(&broadcast, buf, strlen(buf), &multicast_addr, PORT_BROADCAST);

  if(nr_broadcasts++ < 10) {
    ctimer_set(&broadcast_ctimer, 1 * CLOCK_SECOND, send_broadcast, NULL);
  }
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
  printf("found coap server: ");
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
  uip_ipaddr_copy(&remote, sender_addr);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_tester_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

  printf("Starting Coap test.\n");

  if(node_id == 1) {
    rpl_dag_root_init_dag();
    printf("initiating rpl dag\n");
  }

  coap_init_engine();
  rest_init_engine();

  /* Broadcast: used to detect other end */
  uip_create_unspecified(&unspec);
  uip_create_unspecified(&remote);
  uip_create_linklocal_allnodes_mcast(&multicast_addr);
  udp_socket_register(&broadcast, NULL, udp_receiver);
  udp_socket_bind(&broadcast, PORT_BROADCAST);
  udp_socket_connect(&broadcast, NULL, PORT_BROADCAST);
  ctimer_set(&broadcast_ctimer, 1 * CLOCK_SECOND, send_broadcast, NULL); /* Start sending broadcasts */

  while(1) {
    static int current_len = LEN_MIN;

    etimer_set(&periodic_timer, SEND_INTERVAL);
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));

    printf("Requesting uri_len %d\n", current_len);
    send_coap_message(0, current_len);

    current_len++;
    if(current_len > LEN_MAX) {
      current_len = LEN_MIN;
    }
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
//    printf("RF percentage %d\n", (int)rf_percentage);
//    printf("RF channel %d\n", get_rf_channel());
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
