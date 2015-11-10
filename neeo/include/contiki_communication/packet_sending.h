#ifndef _CONTIKI_PACKET_SENDING_H
#define _CONTIKI_PACKET_SENDING_H

#include <sys/select.h>
typedef struct hdr_t {
        uint8_t start;          // always 0x55
        uint8_t message_type;   // for now always 1
        uint8_t message_id;     //identifier of the message
        uint16_t len;           // payload len, can be 0, max 8kB, if exists includes the crc8 at the end
        uint16_t reserved;      // unused now
        uint8_t crc;            // crc of a header
}  __attribute__((packed)) hdr_t;

static uint8_t crc8(const void *vptr, int len);
static int read_bytes(uint8_t * data, uint32_t length, int32_t useconds);
static int32_t receive_from_jn5168(hdr_t * hdr, uint8_t * data, int32_t max_data_length, int32_t timeout_in_ms);
static int32_t send_to_jn5168(hdr_t * hdr, uint8_t * data, int data_length);
static void initializeConnection();

static cyg_io_handle_t jn_handle;
static int fd;
static fd_set fds;

#define MaxBufferLength 360
#define HeaderMagic 0x55
#define WAIT_BETWEEN_SLEEP 50
#define SERIAL_INTERFACE    "/dev/ser1"
#define TTY_INTERFACE       "/dev/tty1"


#endif
