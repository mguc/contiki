/*
 * Copyright (c) 2015, Thingsquare, http://www.thingsquare.com/.
 * All rights reserved.
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

#include "csl.h"
#include "csl-mucha.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 125
#define BECOME_MUCHA_AUTH 1

struct timestamps {
  uint16_t seq;
  csl_symboltimer_clock_t symbols;
  clock_time_t clock;
  int32_t csl_mucha_drift;
};
struct timestamp_diffs {
  int16_t seq;
  int32_t symbols;
  int32_t clock;
  int32_t csl_mucha_drift;
};

static void estimate_drift(void);

static struct timestamps remote_start;
static struct timestamps remote_last;
static struct timestamps my_start;
static struct timestamps my_last;

/*---------------------------------------------------------------------------*/
static struct simple_udp_connection unicast_connection;
/*---------------------------------------------------------------------------*/
PROCESS(test_process, "Estimate drift receiver process");
AUTOSTART_PROCESSES(&test_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr, uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
         const uint8_t *data, uint16_t datalen)
{
  leds_toggle(LEDS_RED);

  if(datalen != sizeof(remote_start)) {
    printf("got warmup\n");
    return;
  }

  if(!remote_start.seq) {
    memcpy((uint8_t*)&remote_start, data, sizeof(remote_start));
  }
  if(remote_start.seq != 1) {
    printf("missed seq 1, instead got %d, reboot try again\n", remote_start.seq); /* you'll have to reboot the devices */
    return;
  }
  memcpy((uint8_t*)&remote_last, data, sizeof(remote_last));

  cooja_debug("TIMESTAMP");
  if(!my_start.seq) {
    my_start.seq++;
    my_start.symbols = CSL_SYMBOLTIMER_NOW();
    my_start.clock = clock_time();
    my_start.csl_mucha_drift = csl_mucha_offset_drift();
  }
  my_last.seq++;
  my_last.symbols = CSL_SYMBOLTIMER_NOW();
  my_last.clock = clock_time();
  my_last.csl_mucha_drift = csl_mucha_offset_drift();

  estimate_drift();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

#if BECOME_MUCHA_AUTH
  csl_mucha_set_authority(CSL_MUCHA_SYNCH_ROOT);

  csl_mucha_channel_blacklist_t all_blacklisted;
  all_blacklisted[0] = (uint32_t) 0xffffffff;
  all_blacklisted[1] = (uint32_t) 0xffffffff;
  csl_mucha_channel_set_blacklist(all_blacklisted);
#endif /* BECOME_MUCHA_AUTH */

  /* Note: set destination to NULL, to allow receiving packets from anyone */
  simple_udp_register(&unicast_connection, UDP_PORT, NULL, UDP_PORT,
                      receiver);

  replong_init();

  etimer_set(&periodic_timer, 30 * CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
    etimer_set(&periodic_timer, 30 * CLOCK_SECOND);

    csl_mlme_request_temporary_period_zero(60*CLOCK_SECOND, 1);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
estimate_drift(void)
{
  if(!my_start.seq || !remote_start.seq) {
    return;
  }

  struct timestamps my_elapsed;
  my_elapsed.symbols = my_last.symbols - my_start.symbols;
  my_elapsed.clock = my_last.clock - my_start.clock;
  my_elapsed.csl_mucha_drift = my_last.csl_mucha_drift - my_start.csl_mucha_drift;

  struct timestamps remote_elapsed;
  remote_elapsed.symbols = remote_last.symbols - remote_start.symbols;
  remote_elapsed.clock = remote_last.clock - remote_start.clock;
  remote_elapsed.csl_mucha_drift = remote_last.csl_mucha_drift - remote_start.csl_mucha_drift;

  /* Compare my and remote elapsed times */
  struct timestamp_diffs drift;
  drift.symbols = (int32_t)(my_elapsed.symbols - remote_elapsed.symbols);
  drift.clock = (int32_t)(my_elapsed.clock - remote_elapsed.clock);
  drift.csl_mucha_drift = (int32_t)(my_elapsed.csl_mucha_drift - remote_elapsed.csl_mucha_drift);

  int64_t ppm;
  printf("Estimated clock drifts (seq=%u, duration %u secs):\n",
         (unsigned int)remote_last.seq, (unsigned int)(my_elapsed.clock / CLOCK_SECOND));

  ppm = (int64_t)drift.symbols * 1000000UL / my_elapsed.symbols;
  printf(" CSL symbols: %ld", (int32_t)drift.symbols);
  printf(" => %ld ppm\n", (int32_t)ppm);

  ppm = (int64_t)drift.clock * 1000000UL / my_elapsed.clock;
  printf(" clock ticks: %ld", (int32_t)drift.clock);
  printf(" => %ld ppm\n", (int32_t)ppm);

  ppm = (int64_t)drift.csl_mucha_drift * 1000000UL / my_elapsed.clock;
  printf("  CSL mucha: %ld", (int32_t)drift.csl_mucha_drift);
  printf(" => %ld ppm\n", (int32_t)ppm);
}
/*---------------------------------------------------------------------------*/
