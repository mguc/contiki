/*
 * Copyright (c) 2015, Thingsquare, http://www.thingsquare.com/.
 * All rights reserved.
 */

#include "contiki.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/leds.h"

#include "net/ip/simple-udp.h"
#include "node-id.h"

#include "replong.h"

#include "net/rime/rimestats.h"

#include "csl.h"
#include "csl-mucha.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 125
#define SEND_INTERVAL (5 * CLOCK_SECOND)

struct timestamps {
  uint16_t seq;
  csl_symboltimer_clock_t symbols;
  clock_time_t clock;
  int32_t csl_mucha_drift;
};

/*---------------------------------------------------------------------------*/
static struct simple_udp_connection unicast_connection;
/*---------------------------------------------------------------------------*/
PROCESS(test_process, "Estimate drift sender process");
AUTOSTART_PROCESSES(&test_process);
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
PROCESS_THREAD(test_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  printf("Starting UDP unicast sender.\n");
  /* Note: set destination to NULL, to allow receiving packets from anyone */
  simple_udp_register(&unicast_connection, UDP_PORT, NULL, UDP_PORT,
                      receiver);

  replong_init();

  csl_mlme_request_temporary_period_zero(60*CLOCK_SECOND, 1);

  const struct replong_neighbor *n;
  static uip_ipaddr_t addr;

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

  csl_mlme_request_temporary_period_zero(60*CLOCK_SECOND, 1);
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    csl_mlme_request_temporary_period_zero(60*CLOCK_SECOND, 1);

#if RIMESTATS_CONF_ON
    if(rimestats.reliabletx != 0) {
      printf("ACKS: %ld/%ld %d%%\n", rimestats.ackrx, rimestats.reliabletx, (int) ((100*rimestats.ackrx)/rimestats.reliabletx));
    }
#endif

    leds_toggle(LEDS_GREEN);

    static int send_warmups = 5;

    if(send_warmups > 0) {
      simple_udp_sendto(&unicast_connection, "tjo", 3, &addr);

      printf("TX warmup %d\n", send_warmups);

      send_warmups--;

    } else {
      /* Update last timestamps */
      static struct timestamps first;
      if(!first.seq) {
        first.seq++;
        first.symbols = CSL_SYMBOLTIMER_NOW();
        first.clock = clock_time();
        first.csl_mucha_drift = csl_mucha_offset_drift();
      }

      static struct timestamps last_periodic;
      last_periodic.seq++;
      cooja_debug("TIMESTAMP");
      last_periodic.symbols = CSL_SYMBOLTIMER_NOW() - first.symbols;
      last_periodic.clock = clock_time() - first.clock;
      last_periodic.csl_mucha_drift = csl_mucha_offset_drift() - first.csl_mucha_drift;

      simple_udp_sendto(&unicast_connection, (uint8_t*)&last_periodic,
                        sizeof(last_periodic), &addr);

      printf("TX timestamp %d\n", last_periodic.seq);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
