#ifndef REST_RESOURCES_H
#define REST_RESOURCES_H

#define COMMAND_STATE_IDLE                  0x00
#define COMMAND_STATE_PENDING               0x01
#define COMMAND_STATE_PENDING_INFINITELY    0x02
#define COMMAND_STATE_READY                 0x03
#define COMMAND_STATE_DATA_READY            0x04
#define COMMAND_STATE_ERROR                 0x05

typedef struct {
  uint32_t state;
  uint32_t result;
} command_state_t;

PROCESS_NAME(rest_server_process);

#endif
