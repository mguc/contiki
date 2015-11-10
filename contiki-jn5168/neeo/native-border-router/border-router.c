/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 *
 * This file is part of the Contiki operating system.
 *
 */
/**
 * \file
 *         border-router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/rpl/rpl.h"
#include "net/ip/simple-udp.h"
#include "net/netstack.h"
#include "dev/slip.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"
#include "rest_resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "log_helper.h"

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

extern long slip_sent;
extern long slip_received;
extern int contiki_argc;
extern char **contiki_argv;
extern const char *slip_config_ipaddr;
extern char *slip_config_addrfile;

static uip_ipaddr_t prefix;
static uint8_t prefix_set;
static uint8_t mac_set;
int discover_enabled;

uip_ipaddr_t tun_address;
CMD_HANDLERS(border_router_cmd_handler);

PROCESS(border_router_process, "Border router process");
PROCESS(discover_process, "Discover process");
AUTOSTART_PROCESSES(&border_router_process,&border_router_cmd_process, &rest_server_process);
// Tha page with information below shuld be implemented as the REST request
/*---------------------------------------------------------------------------*/
// static void
// ipaddr_add(const uip_ipaddr_t *addr)
// {
//   uint16_t a;
//   int i, f;
//   for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
//     a = (addr->u8[i] << 8) + addr->u8[i + 1];
//     if(a == 0 && f >= 0) {
//       if(f++ == 0 && sizeof(buf) - blen >= 2) {
//         buf[blen++] = ':';
//         buf[blen++] = ':';
//       }
//     } else {
//       if(f > 0) {
//         f = -1;
//       } else if(i > 0 && blen < sizeof(buf)) {
//         buf[blen++] = ':';
//       }
//       ADD("%x", a);
//     }
//   }
// }
/*---------------------------------------------------------------------------*/
// static
// PT_THREAD(generate_routes(struct httpd_state *s))
// {
//   static int i;
//   static uip_ds6_route_t *r;
//   static uip_ds6_nbr_t *nbr;

//   PSOCK_BEGIN(&s->sout);

//   SEND_STRING(&s->sout, TOP);

//   blen = 0;
//   ADD("Neighbors<pre>");
//   for(nbr = nbr_table_head(ds6_neighbors);
//       nbr != NULL;
//       nbr = nbr_table_next(ds6_neighbors, nbr)) {
//     ipaddr_add(&nbr->ipaddr);
//     ADD("\n");
//     if(blen > sizeof(buf) - 45) {
//       SEND_STRING(&s->sout, buf);
//       blen = 0;
//     }
//   }

//   ADD("</pre>Routes<pre>");
//   SEND_STRING(&s->sout, buf);
//   blen = 0;
//   for(r = uip_ds6_route_head();
//       r != NULL;
//       r = uip_ds6_route_next(r)) {
//     ipaddr_add(&r->ipaddr);
//     ADD("/%u (via ", r->length);
//     ipaddr_add(uip_ds6_route_nexthop(r));
//     if(r->state.lifetime < 600) {
//       ADD(") %lus\n", (unsigned long)r->state.lifetime);
//     } else {
//       ADD(")\n");
//     }
//     SEND_STRING(&s->sout, buf);
//     blen = 0;
//   }
//   ADD("</pre>");
// }
/*---------------------------------------------------------------------------*/
void
print_addr(const uip_ipaddr_t *addr, char* buf, uint32_t* len)
{
  char *wr_ptr = buf;
  uint8_t ret;

  if(addr == NULL || addr->u8 == NULL) {
    *len = 0;
    return;
  }
  uint16_t a;
  unsigned int i;
  int f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        strncpy(wr_ptr, "::", 2);
        wr_ptr += 2;
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        strncpy(wr_ptr, ":", 1);
        wr_ptr += 1;
      }
      ret = sprintf(wr_ptr, "%x", a);
      wr_ptr += ret;
    }
  }
  wr_ptr+=1;
  *wr_ptr = 0;
  *len = wr_ptr - buf;
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;
  FILE *fd;
  uint32_t len = 64;
  char buf[64];
  uip_ipaddr_t *global_ipaddr = NULL;

  PRINTA("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINTA(" %p: =>", &uip_ds6_if.addr_list[i]);
      uip_debug_ipaddr_print(&(uip_ds6_if.addr_list[i]).ipaddr);
      PRINTA("\n");
      if(state == ADDR_PREFERRED &&
         !uip_is_addr_linklocal(&uip_ds6_if.addr_list[i].ipaddr)) {
        global_ipaddr = &uip_ds6_if.addr_list[i].ipaddr;
      }
    }
  }
  if((slip_config_addrfile != NULL) && (global_ipaddr != NULL))
  {
    fd = fopen(slip_config_addrfile, "w");
    print_addr(global_ipaddr, buf, &len);
    fprintf(fd, "{ \"nbr_web_server\" : \"[%s]\" }\n", buf);
    fclose(fd);
  }
}
/*---------------------------------------------------------------------------*/
static void
request_mac(void)
{
  write_to_slip((uint8_t *)"?M", 2);
}
/*---------------------------------------------------------------------------*/
void
border_router_set_mac(const uint8_t *data)
{
  memcpy(uip_lladdr.addr, data, sizeof(uip_lladdr.addr));
  linkaddr_set_node_addr((linkaddr_t *)uip_lladdr.addr);

  /* is this ok - should instead remove all addresses and
     add them back again - a bit messy... ?*/
  uip_ds6_init();
  rpl_init();

  mac_set = 1;
}
/*---------------------------------------------------------------------------*/
void
border_router_print_stat()
{
  printf("bytes received over SLIP: %ld\n", slip_received);
  printf("bytes sent over SLIP: %ld\n", slip_sent);
}

/*---------------------------------------------------------------------------*/
static void
set_prefix_64(const uip_ipaddr_t *prefix_64)
{
  rpl_dag_t *dag;
  uip_ipaddr_t ipaddr;
  memcpy(&prefix, prefix_64, 16);
  memcpy(&ipaddr, prefix_64, 16);

  prefix_set = 1;
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
  if(dag != NULL) {
    rpl_set_prefix(dag, &prefix, 64);
    PRINTF("created a new RPL dag\n");
  }
}
/*---------------------------------------------------------------------------*/
void discover_callback(struct simple_udp_connection *c,
                       const uip_ipaddr_t *source_addr, uint16_t source_port,
                       const uip_ipaddr_t *dest_addr, uint16_t dest_port,
                       const uint8_t *data, uint16_t datalen)
{
  uint32_t buf_len = 48;
  char buf[48];

  if(!discover_enabled)
    return;

  print_addr(source_addr, buf, &buf_len);
  log_msg(LOG_INFO, "dicovery from: %s ", buf);

  if(strcmp((char*)data, "NBR?") == 0)
  {
    log_msg(LOG_TRACE, "I'm NBR!\n");
    buf_len = 48;
    memcpy(buf, "Y ", 2);
    buf_len -= 2;
    print_addr(&tun_address, buf+2, &buf_len);
    c->remote_port = source_port;
    simple_udp_sendto(c, buf, buf_len+2, source_addr);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(discover_process, ev, data)
{
  static struct simple_udp_connection server_conn;

  PROCESS_BEGIN();

  simple_udp_register(&server_conn, 3200, NULL, 0, discover_callback);

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      log_msg(LOG_ERROR, "tcpip_event\n");
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();
  prefix_set = 0;

  PROCESS_PAUSE();

  PRINTF("RPL-Border router started\n");
  discover_enabled = 0;

  slip_config_handle_arguments(contiki_argc, contiki_argv);

  /* tun init is also responsible for setting up the SLIP connection */
  tun_init();

  while(!mac_set) {
    etimer_set(&et, CLOCK_SECOND);
    request_mac();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  if(slip_config_ipaddr != NULL) {

    if(uiplib_ipaddrconv((const char *)slip_config_ipaddr, &tun_address)) {
      PRINTF("Setting prefix ");
      PRINT6ADDR(&tun_address);
      PRINTF("\n");
      set_prefix_64(&tun_address);
    } else {
      PRINTF("Parse error: %s\n", slip_config_ipaddr);
      exit(0);
    }
  }

#if DEBUG
  print_local_addresses();
#endif

  /* The border router runs with a 100% duty cycle in order to ensure high
     packet reception rates. */
  NETSTACK_MAC.off(1);
  process_start(&discover_process, NULL);

  while(1) {
    etimer_set(&et, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    /* do anything here??? */
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
