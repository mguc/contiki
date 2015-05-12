#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#define T_GET_FW_VERSION  0x02
#define T_PAIR_BRAIN      0x03
#define T_STATUS          0x04
#define T_WIFI_CREDS      0x05
#define T_BRAIN_INFO      0x06
#define T_WEB_SERVER      0x07
#define T_LAST_TYPE     T_BRAIN_INFO

typedef struct  msg_s {
  uint8_t message_type;
  uint8_t message_id;
  uint8_t len;
  uint8_t* data;
} __attribute__((packed)) msg_t;

PROCESS_NAME(serial_parser_process);

void send_msg(msg_t* msg);

#endif /* SERIAL_PROTOCOL_H */