#ifndef _UART_PROTOCOL_H
#define _UART_PROTOCOL_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

#define UART_MUTEX_ERROR -1
#define UART_QUEUE_FULL -2
#define UART_QUEUE_EMPTY -3
#define UART_COMMUNICATION_TIMEOUT -4
#define UART_JN_ERROR -5


#define T_JN_VERSION        0x02
#define T_GET_CP6_LIST      0x03
#define T_STATUS            0x04
#define T_WIFI_CREDS        0x05
#define T_BRAIN_INFO        0x06
#define T_REST_REQUEST      0x07
#define T_FIRMWARE_INFO     0x08
#define T_GET_LOCAL_IP      0x09
#define T_UPDATE_PUSH       0x0A
#define T_PAIR_WITH_CP6     0x0B
#define T_REST_GET          0x11
#define T_REST_POST         0x12
#define T_REST_PUT          0x13
#define T_REST_DELETE       0x14
#define T_ERROR_RESPONSE    0xEE

#define POSSIBLE_STATES_COUNT 4
enum uart_queue_item_state
{
    EMPTY,
    READY_TO_SEND,
    SENT,
    RECEIVED
};

struct uart_queue_item
{
    uint8_t  request_type;
    uint8_t  request_id;

    uint8_t* request_buffer_address;
    uint16_t request_buffer_length;

    enum uart_queue_item_state state;
    bool     is_response_truncated;
    bool     is_error;

    uint8_t* response_buffer_address;
    uint16_t response_data_length;
    uint16_t response_buffer_length;

    // index of previous element having the same state or -1 if it is first such element
    uint32_t previous_position;
    // index of next element having the same state or -1 if it is last such element
    uint32_t next_position;

    uint8_t retries_left;
    time_t  send_time;

    pthread_cond_t response_ready_condition;
};

void init_uart(pthread_mutex_t* communication_mutex, void (*f)(const char*, uint16_t));
void deinit_uart();
int send(uint8_t message_type, uint8_t* input_buffer, uint16_t input_length, uint8_t* output_buffer, uint16_t output_max_size);
int send_and_receive(uint8_t message_type, uint8_t* input_buffer, uint16_t input_length, uint8_t* output_buffer, uint16_t output_max_size, uint16_t * actual_length);
int receive(uint8_t request_id, uint8_t** buffer, uint16_t* length, bool blocking);


#ifdef __cplusplus
}
#endif

#define __cfunc__ (char*)__func__
#endif
