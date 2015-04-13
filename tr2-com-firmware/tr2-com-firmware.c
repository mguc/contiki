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
#define DEBUG 1
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>

#include "net/ip/uip-debug.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "dev/uart0.h"
#include "net/rpl/rpl.h"
#include "lib/ringbuf.h"
#include "version.h"
/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define SERVER_PORT 80
#define INPUTBUFSIZE 400
#define OUTPUTBUFSIZE 400
#define SWAP_BYTES16(__x) ((((__x) & 0xff) << 8) | ((__x) >> 8))
#define TCP_CONNECTED     1
#define TCP_NOT_CONNECTED 0

#define T_PAIR_BRAIN    0x03
#define T_STATUS        0x04
#define T_WIFI_CREDS    0x05
#define T_BRAIN_INFO    0x06
#define T_LAST_TYPE     T_BRAIN_INFO
#define QSF_SENT        0x0001
#define QSF_RECEIVING   0x0002
#define QSF_RECEIVED    0x0004
#define QSF_RESP_READY  0x0008
#define QSF_IDLE        0x0010

typedef struct hdr_s {
        uint8_t start; // always 0x55
        uint8_t message_type; // for now always 1
        uint8_t message_id; //identifier of the message
        uint16_t len; // payload len, can be 0, max 8kB, if exists includes the crc8 at the end
        uint16_t reserved; // unused now
        uint8_t crc; // crc of a header
}  __attribute__((packed)) hdr_t;

typedef struct  msg_s {
  uint8_t message_type;
  uint8_t message_id;
  uint8_t len;
  uint8_t* data;
} __attribute__((packed)) msg_t;

typedef struct query_state_s {
  uint8_t type;
  uint16_t id;
  uint16_t flags;
  uint16_t length;
  uint8_t data[360];
} query_state_t;

/*---------------------------------------------------------------------------*/
static struct uip_udp_conn *server_conn;
static uip_ipaddr_t brain_address;
static uint16_t brain_port;
static struct ringbuf serial_buf;
static uint8_t serial_data[128];
static struct tcp_socket socket;
static uint8_t inputbuf[INPUTBUFSIZE];
static uint8_t outputbuf[OUTPUTBUFSIZE];
static struct etimer et;
static uint8_t tcp_status;
static query_state_t query;
/*---------------------------------------------------------------------------*/
PROCESS(udp_echo_server_process, "UDP echo server process");
PROCESS(config_process, "Configuration process");
PROCESS(serial_parser_process, "Serial protocol parser process");
AUTOSTART_PROCESSES(&udp_echo_server_process);
/*---------------------------------------------------------------------------*/
int serial_input(unsigned char c);
/*---------------------------------------------------------------------------*/
uint8_t
crc8(const void *vptr, int len)
{
        const uint8_t *data = (uint8_t*)vptr;
        unsigned crc = 0;
        int i, j;
        for (j = len; j; j--, data++) {
                crc ^= (*data << 8);
                for(i = 8; i; i--) {
                        if (crc & 0x8000)
                                crc ^= (0x1070 << 3);
                        crc <<= 1;
                }
        }
        return (uint8_t)(crc >> 8);
}
/*---------------------------------------------------------------------------*/
uint8_t
crc8_update(uint16_t crc, uint8_t data)
{
  int i;
  
  crc <<= 8;
  crc ^= (data << 8);
  for(i = 8; i; i--) {
    if (crc & 0x8000)
      crc ^= (0x1070 << 3);
    crc <<= 1;
  }

  return (uint8_t)(crc >> 8);
}
/*---------------------------------------------------------------------------*/
static void
send_msg(msg_t* msg)
{
  uint8_t msgbuff[256];
  hdr_t SerialHeader;
  uint32_t i;

  // filling header
  SerialHeader.start = 0x55;
  SerialHeader.message_type = msg->message_type;
  SerialHeader.message_id = msg->message_id;
  SerialHeader.len = SWAP_BYTES16(msg->len); //((msg->len & 0xFF) << 8) | (msg->len >> 8);
  SerialHeader.reserved = 0;
  SerialHeader.crc = crc8(&SerialHeader, sizeof(SerialHeader) - 1);

  // assemble raw message
  memcpy(msgbuff, &SerialHeader, sizeof(SerialHeader));
  if(msg->len)
    memcpy(msgbuff + sizeof(SerialHeader), msg->data, msg->len);

  // payload crc
  msgbuff[sizeof(SerialHeader) + msg->len] = crc8(msgbuff + sizeof(SerialHeader), msg->len);

  /* pass all data to serial port */
  /* header + payload + crc */
  for(i = 0; i < (sizeof(SerialHeader) + msg->len + 1); i++)
  {
      PRINTF("%02x:", msgbuff[i]);
      uart0_writeb(msgbuff[i]);
  }
  PRINTF("\n");
}
/*---------------------------------------------------------------------------*/

static void
tcpip_handler(void)
{
  uint8_t msgbuff[256];
  msg_t msg;
  uint16_t PayloadLength;
  
  if(uip_newdata()) {
    PayloadLength = uip_datalen();
    PRINTF("%u bytes from [", PayloadLength);
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF("]:%u\n", UIP_HTONS(UIP_UDP_BUF->srcport));

    /* store brain address and port and use it everytime */
    if(!uip_ipaddr_cmp(&brain_address, &UIP_IP_BUF->srcipaddr))
      uip_ipaddr_copy(&brain_address, &UIP_IP_BUF->srcipaddr);
    if(brain_port == 0)
      brain_port = UIP_UDP_BUF->srcport;

    memcpy(msgbuff, uip_appdata, PayloadLength);
    msg.message_type = 0x01;
    msg.message_id = 0; //what ID?
    msg.len = PayloadLength;
    msg.data = msgbuff;

    send_msg(&msg);
  }
}
/*---------------------------------------------------------------------------*/
static int
tcp_input(struct tcp_socket *s, void *ptr,
      const uint8_t *inputptr, int inputdatalen)
{
  int i;
  PRINTF("input %d bytes:", inputdatalen);
  for(i=0; i<inputdatalen; i++)
    PRINTF("%c", inputptr[i]);
  PRINTF("\r\nEOM\r\n\r\n");
  if((query.flags == QSF_SENT) || (query.flags == QSF_RECEIVING))
  {
    PRINTF("adding new data into query buffer\n");
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
  PRINTF("event %d\n", ev);

  switch(ev)
  {
    case TCP_SOCKET_CONNECTED:
      tcp_status = TCP_CONNECTED;
      PRINTF("TCP connected\n");
      break;
    case TCP_SOCKET_CLOSED:
    case TCP_SOCKET_TIMEDOUT:

    case TCP_SOCKET_ABORTED:
      if(query.flags == QSF_RECEIVING)
      {
        PRINTF("setting query flag to READ\n");
        query.flags = QSF_RESP_READY;
      }
      PRINTF("TCP disconnected\n");
      ret = tcp_socket_connect(&socket, &brain_address, 8080);
      PRINTF("TCP re-connect ret: %d\n", ret);
      tcp_status = TCP_NOT_CONNECTED;
      break;
    case TCP_SOCKET_DATA_SENT:
      break;
  }
  PRINTF("query flag = 0x%04x\n", query.flags);
  if(query.flags == QSF_RESP_READY)
  {
    PRINTF("Query response (%u):\n", query.length);
    for(ret = 0; ret<query.length; ret++)
      PRINTF("%c", query.data[ret]);

    PRINTF("\n");
    PRINTF("End of Query response\n");
    query.flags = QSF_IDLE;
    msg.message_type = query.type;
    msg.message_id = query.id;
    msg.data = (uint8_t*)(strstr((char *)query.data, "\r\n\r\n")+4);//query.data;
    msg.len = query.length - (msg.data - query.data);
    PRINTF("Query length (%u):\n", query.length);
    send_msg(&msg);
    PRINTF("Query sent to host!\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_echo_server_process, ev, data)
{
  msg_t* msg;
  msg_t msg_resp;
  uint8_t resp[16];
  int ret;
  PROCESS_BEGIN();
  PRINTF("Starting UDP echo server\n");
  PRINTF("Version: %s\n", version);

  uart0_set_input(serial_input);
  ringbuf_init(&serial_buf, serial_data, sizeof(serial_data));

  // we assume brain address aaaa::1 as the native-border-router on brain should be started like this:
  // border-router.native -B 115200 -s ttyUSB1 aaaa::1/64
  uip_ip6addr(&brain_address, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x0001);
  // uip_create_unspecified(&brain_address);
  brain_port = 3000;
  //brain_port = 0;

  server_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(server_conn, UIP_HTONS(3000));
  PRINTF("Listen port: 3000, TTL=%u\n", server_conn->ttl);

  uip_ds6_init();
  rpl_init();

  tcp_status = TCP_NOT_CONNECTED;
  query.flags = QSF_IDLE;
  query.id = 0;
  query.type = 0;
  query.length = 0;
  ret = tcp_socket_register(&socket, NULL, inputbuf, sizeof(inputbuf), outputbuf, sizeof(outputbuf), tcp_input, tcp_event);
  PRINTF("TCP register ret: %d\n", ret);
  ret = tcp_socket_connect(&socket, &brain_address, 80);
  PRINTF("TCP connect ret: %d\n", ret);

  process_start(&config_process, NULL);
  process_start(&serial_parser_process, NULL);

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
    else if(ev == PROCESS_EVENT_MSG)
    {
      uip_ipaddr_copy(&server_conn->ripaddr, &brain_address);
      server_conn->rport = brain_port;

      memset(resp, 0x20, 16);
      resp[15] = 0;
      msg = (msg_t*)data;
      if(tcp_status == TCP_CONNECTED)
      {
        if(query.flags == QSF_IDLE)
        {
          ret = tcp_socket_send(&socket, msg->data, msg->len);
          query.flags = QSF_SENT;
          query.id = msg->message_id;
          query.type = msg->message_type;
          query.length = 0;
          PRINTF("TCP send ret: %d\n", ret);
          // uip_udp_packet_send(server_conn, msg->data, dataLen);
          // memcpy(resp, "OK", 2);
          msg_resp.message_type = 0;
        }
        else
        {
          PRINTF("TCP busy\n");
          msg_resp.message_type = 0xee;
          memcpy(resp, "BUSY", 4);
        }
      }
      else
      {
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
  static uip_ds6_nbr_t *nbr;
  uint8_t i;
  char resp_buf[40];
  PROCESS_BEGIN();
  PRINTF("Config process started!\n");
  etimer_set(&et, CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_MSG)
    {
      msg_ptr = (msg_t*)data;
      PRINTF("Config polled! %u\n", msg_ptr->message_type);
      if(msg_ptr->message_type== T_STATUS)
      {
        if(tcp_status == TCP_CONNECTED)
        {
          resp_buf[0] = '1';
        }
        else
        {
          resp_buf[0] = '0';
        }
        msg_ptr->data = (uint8_t*)resp_buf;
        msg_ptr->len = 1;
        send_msg(msg_ptr);
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
          msg_ptr->data = (uint8_t*)resp_buf;
          msg_ptr->len = 40;
        }
        else
        {
          msg_ptr->message_type = 0xee;
          msg_ptr->len = 0;
        }
        send_msg(msg_ptr);
      }
      else if(msg_ptr->message_type == T_PAIR_BRAIN)
      {
        // open TCP/IP connection?
        send_msg(msg_ptr);
      }
    }
    else if(ev == PROCESS_EVENT_TIMER)
    {
      for(nbr = nbr_table_head(ds6_neighbors);
          nbr != NULL;
          nbr = nbr_table_next(ds6_neighbors, nbr)) {
        PRINTF("Neighbor: ");
        for(i=0; i<7; i++)
          PRINTF("%04x:", nbr->ipaddr.u16[i]);
        PRINTF("%04x\n", nbr->ipaddr.u16[7]);
      }
      etimer_set(&et, CLOCK_SECOND);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
enum ParserStateEnum {
    WaitForSync = 0,
    GetType,
    GetID,
    GetLengthLower,
    GetLengthUpper,
    GetReservedLower,
    GetReservedUpper,
    GetHeaderCRC,
    GetPayload,
    GetPayloadCRC
} ParserState = WaitForSync;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(serial_parser_process, ev, data)
{
  uint8_t MsgReady;
  uint8_t content[128];
  int c;
  uint16_t i;
  uint8_t crc;
  msg_t msg;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_POLL)
    {
      // PRINTF("parser polled!\n");
      while((c = ringbuf_get(&serial_buf)) != -1) {
        // PRINTF("got from ringbuf 0x%02x\n", c);
        switch(ParserState){
        case WaitForSync:
          MsgReady = 0;
          i = 0;
          if(c != 0x55)
            break;
          PRINTF("sync ok\n");
          ParserState = GetType;
          crc = crc8_update(0, c);
          break;
        case GetType:
          if(c > 0xf0)
          {
            ParserState = WaitForSync;
            break;
          }
          msg.message_type = c;
          PRINTF("type is 0x%02x\n", msg.message_type);
          ParserState = GetID;
          crc = crc8_update(crc, c);
          break;
        case GetID:
          msg.message_id = c;
          ParserState = GetLengthLower;
          crc = crc8_update(crc, c);
          break;
        case GetLengthLower:
          msg.len = c;
          ParserState = GetLengthUpper;
          crc = crc8_update(crc, c);
          break;
        case GetLengthUpper:
          c = c << 8;
          msg.len |= c;
          PRINTF("size is 0x%02x (%u)\n", msg.len, msg.len);
          ParserState = GetReservedLower;
          crc = crc8_update(crc, c);
          break;
        case GetReservedLower:
          ParserState = GetReservedUpper;
          crc = crc8_update(crc, c);
          break;
        case GetReservedUpper:
          ParserState = GetHeaderCRC;
          crc = crc8_update(crc, c);
          break;
        case GetHeaderCRC:
          PRINTF("crc 0x%02x recieved is 0x%02x\n", crc, c);
          if(crc != c)
            ParserState = WaitForSync;
          else {
            if(msg.len) {
              ParserState = GetPayload;
            }
            else {
              ParserState = GetPayloadCRC;
            }
          }
          crc = 0;
          break;
        case GetPayload:
          content[i++] = (uint8_t)c;
          PRINTF("data 0x%02x\n", c);
          if(i == msg.len) {
            ParserState = GetPayloadCRC;
          }
          crc = crc8_update(crc, c);
          break;
        case GetPayloadCRC:
          PRINTF("PayloadCRC is 0x%02x received 0x%02x\n", crc, c);
          if(crc != c) {
            ParserState = WaitForSync;
            break;
          }
          msg.data = content;
          MsgReady = 1;
          break;
        }

        if(!MsgReady)
          continue;
        PRINTF("Post msg type 0x%02x\n", msg.message_type);
        switch(msg.message_type){
        case T_PAIR_BRAIN:
        case T_STATUS:
        case T_BRAIN_INFO:
          process_post_synch(&config_process, PROCESS_EVENT_MSG, &msg);
          break;
        case T_WIFI_CREDS:
          process_post_synch(&udp_echo_server_process, PROCESS_EVENT_MSG, &msg);
          break;
        default:
          break;
        }
        memset(&msg, 0, sizeof(msg_t));
        ParserState = WaitForSync;
        MsgReady = 0;
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
int serial_input(unsigned char c)
{
  if(ringbuf_put(&serial_buf, c) == 0){
    //Overflow
    return -1;
  }
  process_poll(&serial_parser_process);

  return 0;
}
/**
 * @}
 * @}
 */
