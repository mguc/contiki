#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#define T_GET_FW_VERSION  0x02
#define T_GET_CP6_LIST    0x03
#define T_STATUS          0x04
#define T_WIFI_CREDS      0x05
#define T_BRAIN_INFO      0x06
#define T_REST_REQUEST    0x07
#define T_FIRMWARE_INFO   0x08
#define T_GET_LOCAL_IP    0x09
#define T_UPDATE_PUSH     0x0A
#define T_PAIR_WITH_CP6   0x0B
#define T_REST_GET        0x11
#define T_REST_POST       0x12
#define T_REST_PUT        0x13
#define T_REST_DELETE     0x14
#define T_TRIGGER_ACTION  0x15
#define T_DEBUG			  0x16
#define T_ERROR_RESPONSE  0xEE

typedef struct  msg_s {
  uint8_t type;
  uint8_t id;
  uint16_t len;
  uint8_t* data;
} msg_t;

PROCESS_NAME(serial_parser_process);

void send_msg(msg_t* msg);

#endif /* SERIAL_PROTOCOL_H */
