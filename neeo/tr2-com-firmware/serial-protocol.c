#include <string.h>
#include "contiki.h"
#include "contiki-lib.h"
#include "dev/uart0.h"
#include "lib/ringbuf.h"
#include "serial-protocol.h"
#include "tr2-common.h"
#define DEBUG_LEVEL DEBUG_NONE
#include "log_helper.h"

#define SWAP_BYTES16(__x) ((((__x) & 0xff) << 8) | ((__x) >> 8))
#define PAYLOAD_MAX_LEN 256

typedef struct hdr_s {
        uint8_t start; // always 0x55
        uint8_t type; // for now always 1
        uint8_t id; //identifier of the message
        uint16_t len; // payload len, can be 0, max 8kB, if exists includes the crc8 at the end
        uint16_t reserved; // unused now
        uint8_t crc; // crc of a header
}  __attribute__((packed)) hdr_t;

static uint8_t payload[PAYLOAD_MAX_LEN];
static uint8_t MsgReady;
static int payload_index;
static uint8_t crc;
static msg_t msg;
static struct ringbuf serial_buf;
static uint8_t serial_buf_data[128];

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
PROCESS(serial_parser_process, "Serial protocol parser process");
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
int serial_input(unsigned char c)
{
  if(ringbuf_put(&serial_buf, c) == 0){
    //Overflow
    return -1;
  }
  process_poll(&serial_parser_process);

  return 0;
}
void send_msg(msg_t* msg)
{
  uint8_t hdr_buf[16];
  hdr_t SerialHeader;
  uint8_t payload_crc = 0;
  uint32_t i;

  // filling header
  SerialHeader.start = 0x55;
  SerialHeader.type = msg->type;
  SerialHeader.id = msg->id;
  SerialHeader.len = SWAP_BYTES16(msg->len); //((msg->len & 0xFF) << 8) | (msg->len >> 8);
  SerialHeader.reserved = 0;
  SerialHeader.crc = crc8(&SerialHeader, sizeof(SerialHeader) - 1);

  memcpy(hdr_buf, &SerialHeader, sizeof(SerialHeader));
  payload_crc = crc8(msg->data, msg->len);

  /* pass all data to serial port */
  /* header + payload + crc */
  INFOT("MSG: ");
  for(i = 0; i < sizeof(SerialHeader); i++)
  {
      INFO("%02x ", hdr_buf[i]);
      uart0_writeb(hdr_buf[i]);
  }
  for(i = 0; i < msg->len; i++)
  {
    INFO("%02x ", msg->data[i]);
    uart0_writeb(msg->data[i]);
  }
  INFO("%02x ", payload_crc);
  uart0_writeb(payload_crc);
  INFO("\n");
}

PROCESS_THREAD(serial_parser_process, ev, data)
{
  int c, j;

  PROCESS_BEGIN();

  uart0_set_input(serial_input);
  ringbuf_init(&serial_buf, serial_buf_data, sizeof(serial_buf_data));

  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_POLL)
    {
      while((c = ringbuf_get(&serial_buf)) != -1) {
        switch(ParserState){
        case WaitForSync:
          MsgReady = 0;
          payload_index = 0;
          if(c != 0x55)
            break;
          ParserState = GetType;
          crc = crc8_update(0, c);
          break;
        case GetType:
          if(c > 0xf0)
          {
            ParserState = WaitForSync;
            break;
          }
          msg.type = c;
          ParserState = GetID;
          crc = crc8_update(crc, c);
          break;
        case GetID:
          msg.id = c;
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
          payload[payload_index++] = (uint8_t)c;
          if(payload_index >= PAYLOAD_MAX_LEN) {
            ERRORT("PARSE: Overflow! payload_index = %d\n", payload_index);
            ParserState = WaitForSync;
            break;
          }
          if(payload_index == msg.len) {
            ParserState = GetPayloadCRC;
          }
          crc = crc8_update(crc, c);
          break;
        case GetPayloadCRC:
          if(crc != c) {
            ParserState = WaitForSync;
            break;
          }
          msg.data = payload;
          MsgReady = 1;
          break;
        }

        if(!MsgReady)
          continue;
        INFOT("RCV T:%02x ID:%02x ", msg.type, msg.id);
        INFO("L:%3u", msg.len);
        if(msg.len)
        {
          INFO(" D: ");
          for(j=0; j<msg.len; j++)
            INFO("%02x ", msg.data[j]);
        }
        INFO("\n");
        switch(msg.type){
        case T_GET_CP6_LIST:
          process_post_synch(&discover_process, PROCESS_EVENT_MSG, &msg);
        case T_STATUS:
        case T_BRAIN_INFO:
        case T_GET_FW_VERSION:
        case T_GET_LOCAL_IP:
        case T_PAIR_WITH_CP6:
          process_post_synch(&config_process, PROCESS_EVENT_MSG, &msg);
          break;
        case T_FIRMWARE_INFO:
          process_post_synch(&query_process, PROCESS_EVENT_MSG, &msg);
          break;
        case T_WIFI_CREDS:
        case T_REST_GET:
        case T_REST_POST:
        case T_REST_PUT:
        case T_REST_DELETE:
        case T_TRIGGER_ACTION:
          process_post_synch(&coap_process, PROCESS_EVENT_MSG, &msg);
          break;
        default:
          // TODO: send msg with bad type
          break;
        }
        memset(&msg, 0, sizeof(msg_t));
        ParserState = WaitForSync;
        MsgReady = 0;
        payload_index = 0;
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
