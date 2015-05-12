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

#include "net/ip/uip-debug.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "net/rpl/rpl.h"
#include "lib/ringbuf.h"
#include "version.h"
#include "serial-protocol.h"
#include "tr2-common.h"
/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define SERVER_PORT 80
#define INPUTBUFSIZE 400
#define OUTPUTBUFSIZE 400
#define TCP_CONNECTED     1
#define TCP_NOT_CONNECTED 0
#define BRAIN_ADDRESS   80
#define QSF_SENT        0x0001
#define QSF_RECEIVING   0x0002
#define QSF_RECEIVED    0x0004
#define QSF_RESP_READY  0x0008
#define QSF_IDLE        0x0010
#define FW_MAJOR_VERSION    "1"

typedef struct query_state_s {
  uint8_t type;
  uint16_t id;
  uint16_t flags;
  uint16_t length;
  uint8_t data[360];
} query_state_t;
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t brain_address;
static struct tcp_socket socket;
static uint8_t inputbuf[INPUTBUFSIZE];
static uint8_t outputbuf[OUTPUTBUFSIZE];
static struct etimer et;
static uint8_t tcp_status;
static query_state_t query;
/*---------------------------------------------------------------------------*/
PROCESS(query_process, "Query process");
PROCESS(config_process, "Configuration process");
AUTOSTART_PROCESSES(&query_process);
/*---------------------------------------------------------------------------*/
static int
tcp_input(struct tcp_socket *s, void *ptr,
      const uint8_t *inputptr, int inputdatalen)
{
  int i;
  PRINTF("TCP Input: received %d bytes:\n", inputdatalen);
  if(inputdatalen)
  {
    for(i=0; i<inputdatalen; i++)
      PRINTF("%c", inputptr[i]);
    PRINTF("\n\n");
  }
  if((query.flags == QSF_SENT) || (query.flags == QSF_RECEIVING))
  {
    PRINTF("TCP Input: adding new data into query buffer\n");
    memcpy(query.data + query.length, inputptr, inputdatalen);
    query.length += inputdatalen;
    query.flags = QSF_RECEIVING;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
tcp_event(struct tcp_socket *s, void *ptr,
      tcp_socket_event_t ev)
{
  int ret;
  msg_t msg;

  switch(ev)
  {
    case TCP_SOCKET_CONNECTED:
      tcp_status = TCP_CONNECTED;
      PRINTF("TCP Event: connected\n");
      break;
    case TCP_SOCKET_CLOSED:
    case TCP_SOCKET_TIMEDOUT:

    case TCP_SOCKET_ABORTED:
      if(query.flags == QSF_RECEIVING)
      {
        PRINTF("TCP Event: setting query flag to READ\n");
        query.flags = QSF_RESP_READY;
      }
      PRINTF("TCP Event: disconnected (%u)\n", ev);
      ret = tcp_socket_connect(&socket, &brain_address, BRAIN_ADDRESS);
      PRINTF("TCP Event: re-connect ret: %d\n", ret);
      tcp_status = TCP_NOT_CONNECTED;
      break;
    case TCP_SOCKET_DATA_SENT:
      break;
  }
  PRINTF("TCP Event: query flag = 0x%04x\n", query.flags);
  if(query.flags == QSF_RESP_READY)
  {
    PRINTF("TCP Event: query response (%3u) ######################\n", query.length);
    for(ret = 0; ret<query.length; ret++)
      PRINTF("%c", query.data[ret]);
    PRINTF("\n");
    PRINTF("TCP Event: end of query response #####################\n");
    query.flags = QSF_IDLE;
    msg.message_type = query.type;
    msg.message_id = query.id;
    msg.data = (uint8_t*)(strstr((char *)query.data, "\r\n\r\n")+4);//query.data;
    msg.len = query.length - (msg.data - query.data);
    send_msg(&msg);
  }
}
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
PROCESS_THREAD(query_process, ev, data)
{
  msg_t* msg;
  msg_t msg_resp;
  uint8_t resp[16];
  char buf[40];
  uint32_t buf_len;
  int ret, i;

  PROCESS_BEGIN();
  PRINTF("INIT: Starting query process\n");
  PRINTF("INIT: Version: %s.%s\n", FW_MAJOR_VERSION, version);

  process_start(&config_process, NULL);
  process_start(&serial_parser_process, NULL);

  // we assume brain address aaaa::1 as the native-border-router on brain should be started like this:
  // border-router.native -B 115200 -s ttyUSB1 aaaa::1/64
  uip_ip6addr(&brain_address, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x0001);
  // uip_create_unspecified(&brain_address);

  uip_ds6_init();
  rpl_init();
  print_addr(&brain_address, buf, &buf_len);
  PRINTF("INIT: Brain address: %s\n", buf);

  tcp_status = TCP_NOT_CONNECTED;
  query.flags = QSF_IDLE;
  query.id = 0;
  query.type = 0;
  query.length = 0;
  ret = tcp_socket_register(&socket, NULL, inputbuf, sizeof(inputbuf), outputbuf, sizeof(outputbuf), tcp_input, tcp_event);
  PRINTF("INIT: TCP register ret: %d\n", ret);
  ret = tcp_socket_connect(&socket, &brain_address, BRAIN_ADDRESS);
  PRINTF("INIT: TCP connect ret: %d\n", ret);

  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_MSG)
    {
      memset(resp, 0x20, 16);
      resp[15] = 0;
      msg = (msg_t*)data;
      if(tcp_status == TCP_CONNECTED)
      {
        if(query.flags == QSF_IDLE)
        {
          PRINTF("QUERY (%03u): ###############################\n", msg->len);
          for(i=0; i<msg->len; i++)
            PRINTF("%c", msg->data[i]);
          PRINTF("QUERY END ###################################\n");
          ret = tcp_socket_send(&socket, msg->data, msg->len);
          query.flags = QSF_SENT;
          query.id = msg->message_id;
          query.type = msg->message_type;
          query.length = 0;
          PRINTF("Query: TCP send ret: %d\n", ret);
          // uip_udp_packet_send(server_conn, msg->data, dataLen);
          // memcpy(resp, "OK", 2);
          msg_resp.message_type = 0;
        }
        else
        {
          PRINTF("Query: TCP busy\n");
          msg_resp.message_type = 0xee;
          memcpy(resp, "BUSY", 4);
        }
      }
      else
      {
        PRINTF("Query: Not connected\n");
        msg_resp.message_type = 0xee;
        memcpy(resp, "NOT CONNECTED", 13);
      }

      if(msg_resp.message_type)
      {
        msg_resp.message_id = msg->message_id;
        msg_resp.len = 16;
        msg_resp.data = resp;
        send_msg(&msg_resp);
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(config_process, ev, data)
{
  msg_t* msg_ptr;
  static msg_t msg_buf;
  static uip_ds6_nbr_t *nbr;
  uint8_t i;
  char resp_buf[40];
  radio_value_t rssi_val = 0;

  PROCESS_BEGIN();
  PRINTF("INIT: Starting config process\n");
  etimer_set(&et, CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_MSG)
    {
      msg_ptr = (msg_t*)data;
      msg_buf.message_type = msg_ptr->message_type;
      msg_buf.message_id = msg_ptr->message_id;
      if(msg_ptr->message_type == T_STATUS)
      {
        NETSTACK_CONF_RADIO.get_value(RADIO_PARAM_RSSI, &rssi_val);
        PRINTF("RSSI: %d dBm\n", rssi_val);
        if(rssi_val == 0)
          resp_buf[0] = '0';
        else if(rssi_val > 0)
          resp_buf[0] = '4';
        else if((rssi_val > -9) && (rssi_val < 0))
          resp_buf[0] = '3';
        else if((rssi_val > -19) && (rssi_val < -10))
          resp_buf[0] = '2';
        else if((rssi_val > -29) && (rssi_val < -20))
          resp_buf[0] = '1';
        else
          resp_buf[0] = '0';

        msg_buf.data = (uint8_t*)resp_buf;
        msg_buf.len = 1;
        send_msg(&msg_buf);
      }
      else if(msg_ptr->message_type == T_BRAIN_INFO)
      {
        if(tcp_status == TCP_CONNECTED)
        {
          sprintf(resp_buf, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                              brain_address.u8[0], brain_address.u8[1],
                              brain_address.u8[2], brain_address.u8[3],
                              brain_address.u8[4], brain_address.u8[5],
                              brain_address.u8[6], brain_address.u8[7],
                              brain_address.u8[8], brain_address.u8[9],
                              brain_address.u8[10], brain_address.u8[11],
                              brain_address.u8[12], brain_address.u8[13],
                              brain_address.u8[14], brain_address.u8[15]);
          resp_buf[39] = 0;
          msg_buf.data = (uint8_t*)resp_buf;
          msg_buf.len = 40;
        }
        else
        {
          msg_buf.message_type = 0xee;
          msg_buf.data = NULL;
          msg_buf.len = 0;
        }
        send_msg(&msg_buf);
      }
      else if(msg_ptr->message_type == T_PAIR_BRAIN)
      {
        // TODO: start pair process, open TCP/IP connection?
        msg_buf.data = NULL;
        msg_buf.len = 0;
        send_msg(&msg_buf);
      }
      else if(msg_ptr->message_type == T_GET_FW_VERSION)
      {
        strcpy(resp_buf, FW_MAJOR_VERSION);
        msg_buf.data = (uint8_t*)resp_buf;
        msg_buf.len = strlen(FW_MAJOR_VERSION) + 1;
        send_msg(&msg_buf);
      }
    }
    else if(ev == PROCESS_EVENT_TIMER)
    {
      if(nbr_table_head(ds6_neighbors))
        PRINTF("Neighbor list:\n");
      for(nbr = nbr_table_head(ds6_neighbors);
          nbr != NULL;
          nbr = nbr_table_next(ds6_neighbors, nbr)) {
        PRINTF("\t\t");
        for(i=0; i<7; i++)
          PRINTF("%04x:", nbr->ipaddr.u16[i]);
        PRINTF("%04x\n", nbr->ipaddr.u16[7]);
      }
      etimer_set(&et, CLOCK_SECOND);
    }
  }
  PROCESS_END();
}
