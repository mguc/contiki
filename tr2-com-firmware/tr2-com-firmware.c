/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** \addtogroup cc2538-examples
 * @{
 *
 * \defgroup cc2538-echo-server cc2538dk UDP Echo Server Project
 *
 *  Tests that a node can correctly join an RPL network and also tests UDP
 *  functionality
 * @{
 *
 * \file
 *  An example of a simple UDP echo server for the cc2538dk platform
 */
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>

#define DEBUG 0
#include "net/ip/uip-debug.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "net/rpl/rpl.h"
#include "lib/ringbuf.h"
/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])

#define MAX_PAYLOAD_LEN 120
/*---------------------------------------------------------------------------*/
static struct uip_udp_conn *server_conn;
static char buf[MAX_PAYLOAD_LEN];
static uint16_t len;
static uip_ipaddr_t brain_address;
static uint16_t brain_port;
static struct ringbuf serial_buf;
static uint8_t serial_data[128];
volatile uint8_t frame_ready;
/*---------------------------------------------------------------------------*/
PROCESS(udp_echo_server_process, "UDP echo server process");
AUTOSTART_PROCESSES(&udp_echo_server_process);
/*---------------------------------------------------------------------------*/
int serial_input(unsigned char c);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  uint32_t i;
  memset(buf, 0, MAX_PAYLOAD_LEN);
  if(uip_newdata()) {
    len = uip_datalen();
    memcpy(buf, uip_appdata, len);
    PRINTF("%u bytes from [", len);
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF("]:%u\n", UIP_HTONS(UIP_UDP_BUF->srcport));

    /* store brain address and port and use it everytime */
    if(!uip_ipaddr_cmp(&brain_address, &UIP_IP_BUF->srcipaddr))
      uip_ipaddr_copy(&brain_address, &UIP_IP_BUF->srcipaddr);
    if(brain_port == 0)
      brain_port = UIP_UDP_BUF->srcport;

    /* pass all data to serial port */
    for(i = 0; i < len; i++)
    {
      putchar(buf[i]);
    }


  }
  return;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_echo_server_process, ev, data)
{
  uint8_t buf[128];
  uint32_t len, i;
  PROCESS_BEGIN();
  PRINTF("Starting UDP echo server\n");

  frame_ready = 0;
  uart0_set_input(serial_input);
  ringbuf_init(&serial_buf, serial_data, sizeof(serial_data));

  // we assume brain address aaaa::1 as the native-border-router on brain should be started like this:
  // border-router.native -B 115200 -s ttyUSB1 aaaa::1/64
  uip_ip6addr(&brain_address, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x0001);   
  // uip_create_unspecified(&brain_address);
  brain_port = 3000;
  // brain_port = 0;

  server_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(server_conn, UIP_HTONS(3000));

  PRINTF("Listen port: 3000, TTL=%u\n", server_conn->ttl);

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
    else if(ev == PROCESS_EVENT_POLL)
    {
      if(frame_ready)
      {
        len = ringbuf_elements(&serial_buf);
        for(i = 0; i < len; i++)
          buf[i] = ringbuf_get(&serial_buf);
        uip_ipaddr_copy(&server_conn->ripaddr, &brain_address);
        server_conn->rport = brain_port;
        uip_udp_packet_send(server_conn, buf, len);
        PRINTF("Packet sent! Len = %u\n", len);
        frame_ready = 0;
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
int serial_input(unsigned char c)
{
  static int pattern_level = 0;

  if(ringbuf_put(&serial_buf, c) == 0)
    /* Ooops! Ringbuffer overflow! Stop reading! */
    return -1;

  /* search for end of frame pattern: \n\r\n\r */
  if(c == '\n' && (pattern_level == 0 || pattern_level == 2))
    pattern_level++;
  else if(c == '\r' && (pattern_level == 1 || pattern_level == 3))
    pattern_level++;
  else
    pattern_level = 0;

  if(pattern_level == 4)
  {
    frame_ready = 1;
    process_poll(&udp_echo_server_process);
    pattern_level = 0;
  }

  /* everything is OK, continue */
  return 0;
}
/**
 * @}
 * @}
 */
