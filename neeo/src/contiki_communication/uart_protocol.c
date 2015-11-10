#include "contiki_communication/uart_protocol.h"
#include "contiki_communication/uart_protocol_inner.h"
#include "contiki_communication/communication.h"
#include "log_helper.h"
#include "msleep.h"

#include <string.h>
#include <time.h>

#define UART_QUEUE_SIZE 256
#define UART_RESPONSE_BUFFER_SIZE 512
// in ms
#define UART_READ_TIMEOUT 100
#define RESEND_RETRIES 1
// in s!
#define UART_SEND_TIMEOUT 10

#if 1

#undef LOG_TRACE
#define LOG_TRACE LOG_IGNORE

#endif

// index of the first element of given state or -1 if there are no such elements
static uint32_t first[POSSIBLE_STATES_COUNT];

static pthread_mutex_t* JNSerialMutex;

static struct uart_queue_item uart_queue[UART_QUEUE_SIZE];

static pthread_mutex_t uart_queue_mutex;

static pthread_t uart_thread;

static void (*JN_update_callback)(const char* xml, uint16_t xml_length);

void init_uart(pthread_mutex_t* communication_mutex, void (*f)(const char*, uint16_t))
{
    JNSerialMutex = communication_mutex;

    pthread_mutexattr_t input_attr;
	pthread_mutexattr_init(&input_attr);
	pthread_mutex_init(&uart_queue_mutex, &input_attr);

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN);

    first[EMPTY] = 0;
    first[READY_TO_SEND] = -1;
    first[SENT] = -1;
    first[RECEIVED] = -1;

    if(f)
        JN_update_callback = f;
    else
        JN_update_callback = NULL;

    int i;
    for (i = 0; i < UART_QUEUE_SIZE; i++)
    {
        uart_queue[i].state = EMPTY;
        uart_queue[i].previous_position = i - 1;
        uart_queue[i].next_position = (i == UART_QUEUE_SIZE - 1) ? -1 : i + 1;

        pthread_cond_init(&uart_queue[i].response_ready_condition, NULL);
    }

    log_msg(LOG_INFO, "main", "Spawning UartThread");
    pthread_create(&uart_thread, &thread_attr, &JNUartThread, NULL);
}

void deinit_uart()
{
    int i;
    for (i = 0; i < UART_QUEUE_SIZE; i++)
    {
        pthread_cond_destroy(&uart_queue[i].response_ready_condition);
    }

    pthread_join(uart_thread, NULL);
}

int send(uint8_t message_type, uint8_t* input_buffer, uint16_t input_length, uint8_t* output_buffer, uint16_t output_max_size)
{
    log_msg(LOG_TRACE, "UartSender", "Sending message of type: %d", message_type);
    if (pthread_mutex_lock(&uart_queue_mutex) == 0)
    {
        if (first[EMPTY] == -1)
        {
            pthread_mutex_unlock(&uart_queue_mutex);
            log_msg(LOG_TRACE, "UartSender", "Queue is full, dropping.");
            // queue is full, so we return error code
            return UART_QUEUE_FULL;
        }

        int position = first[EMPTY];

        uart_queue[position].request_type            = message_type;
        uart_queue[position].request_id              = position;
        uart_queue[position].request_buffer_address  = input_buffer;
        uart_queue[position].request_buffer_length   = input_length;
        uart_queue[position].response_buffer_address = output_buffer;
        uart_queue[position].response_buffer_length  = output_max_size;

        uart_queue[position].retries_left            = RESEND_RETRIES;
        change_state(&uart_queue[position], READY_TO_SEND);

        log_msg(LOG_TRACE, "UartSender", "Message was queued with id %d", position);

        pthread_mutex_unlock(&uart_queue_mutex);
        return position;
    }

    return UART_MUTEX_ERROR;
}

int send_and_receive(uint8_t message_type, uint8_t* input_buffer, uint16_t input_length, uint8_t* output_buffer, uint16_t output_max_size, uint16_t * actual_length)
{
    int maxNumberOfTries = 3;
    log_msg(LOG_TRACE, __cfunc__, "Sending a message of type %d.", message_type);
    int id = send(message_type, input_buffer, input_length, output_buffer, output_max_size);
    if (id < 0)
    {
        log_msg(LOG_TRACE, __cfunc__, "Error while sending request of type %d: %d\n", message_type, id);
        return id;
    }
    while(maxNumberOfTries--)
    {
        log_msg(LOG_TRACE, __cfunc__, "Waiting for jn5168 response of type %d...", message_type);
        int result = receive(id, NULL, actual_length, true);
        if (result == UART_QUEUE_EMPTY)
        {
            log_msg(LOG_TRACE, __cfunc__, "No response ready for request of type %d, will try again in 500 ms.", message_type);
            msleep(500);
            continue;
        }
        else if (result == UART_COMMUNICATION_TIMEOUT)
        {
            log_msg(LOG_ERROR, __cfunc__, "Timed out while waiting for response of type %d.", message_type);
            return -1;
        }
        else if (result == 0)
        {
            log_msg(LOG_TRACE, __cfunc__, "Response received.");
            return 0;
        }
        else if (result == UART_JN_ERROR)
        {
            //Add \0 to stringify
            if(actual_length > 0)
            {
                output_buffer[*actual_length == output_max_size ? *actual_length - 1 : *actual_length] = '\0';
                log_msg(LOG_ERROR, __cfunc__, "Error: %s.", output_buffer);
            }
            else
            {
                log_msg(LOG_ERROR, __cfunc__, "Communication failed.");
            }
            return -1;
        }
        else
        {
            log_msg(LOG_ERROR, __cfunc__, "Some other error while checking for response of type %d: %d.", message_type, result);
            return -1;
        }

    }
    log_msg(LOG_ERROR, __cfunc__, "No response ready for request of type %d, aborting.", message_type);
    return -1;
}

int receive(uint8_t request_id, uint8_t** buffer, uint16_t* length, bool blocking)
{
    if (pthread_mutex_lock(&uart_queue_mutex) == 0)
    {
        if (blocking)
        {
            while (uart_queue[request_id].state != RECEIVED)
            {
                pthread_cond_wait(&uart_queue[request_id].response_ready_condition, &uart_queue_mutex);
            }
        }

        if (uart_queue[request_id].state == RECEIVED)
        {
            if (buffer != NULL)
            {
                *buffer = uart_queue[request_id].response_buffer_address;
            }
            *length = uart_queue[request_id].response_data_length;

            int result = uart_queue[request_id].is_error ? UART_COMMUNICATION_TIMEOUT : 0;
            change_state(&uart_queue[request_id], EMPTY);

            if (uart_queue[request_id].request_type == T_ERROR_RESPONSE)
            {
                 log_msg(LOG_TRACE, __cfunc__, "Setting error status for message of type %d.", uart_queue[request_id].request_type);
                result = UART_JN_ERROR;
            }
            pthread_mutex_unlock(&uart_queue_mutex);
            return result;
        }

        pthread_mutex_unlock(&uart_queue_mutex);
        return UART_QUEUE_EMPTY;
    }

    return UART_MUTEX_ERROR;
}

static void *JNUartThread(void* x)
{
    int32_t bytes_read;
    uint8_t local_buffer[UART_RESPONSE_BUFFER_SIZE];

    while(1)
    {
        // log_msg(LOG_TRACE, "UartThread", "Uart thread iteration started");
        if (pthread_mutex_lock(&uart_queue_mutex) == 0)
        {
            // log_msg(LOG_TRACE, "UartThread", "About to send new requests...");
            while (first[READY_TO_SEND] != -1)
            {
                struct uart_queue_item* item = &uart_queue[first[READY_TO_SEND]];

                int result = -1;
                if (pthread_mutex_lock(JNSerialMutex) == 0)
                {
                    log_msg(LOG_TRACE, "UartThread", "Sending message with id: %d", item->request_id);
                    result = jn5168_send(item->request_type, item->request_id, item->request_buffer_address, item->request_buffer_length);
                    pthread_mutex_unlock(JNSerialMutex);
                }

                if (result != 0)
                {
                    log_msg(LOG_ERROR, "UartThread", "Error while sending request: %d", result);
                    break;
                }

                time(&item->send_time);
                change_state(item, SENT);
            }

            // log_msg(LOG_TRACE, "UartThread", "Looking for timed out requests...");
            // iterate over `sent` and check if it is not timed-out
            int current_position;
            time_t now = time(NULL);
            for (current_position = first[SENT]; current_position != -1; current_position = uart_queue[current_position].next_position)
            {
                double diff = difftime(now, uart_queue[current_position].send_time);
                if (diff > UART_SEND_TIMEOUT)
                {
                    if(uart_queue[current_position].retries_left == 0)
                    {
                        uart_queue[current_position].is_error = true;
                        change_state(&uart_queue[current_position], RECEIVED);
                        // we should not evaluate further, as uart_queue[current_position].next_position points to element of a different state!
                        break;
                    }
                    else
                    {
                        uart_queue[current_position].retries_left--;
                        change_state(&uart_queue[current_position], READY_TO_SEND);
                        // we should not evaluate further, as uart_queue[current_position].next_position points to element of a different state!
                        break;
                    }
                }
            }

            pthread_mutex_unlock(&uart_queue_mutex);
        }

        int id = -1;
        int result_type;
        if (pthread_mutex_lock(JNSerialMutex) == 0)
        {
            log_msg(LOG_TRACE, "UartThread", "Waiting for data...");
            id = jn5168_receive(local_buffer, UART_RESPONSE_BUFFER_SIZE, &bytes_read, UART_READ_TIMEOUT, &result_type);
            pthread_mutex_unlock(JNSerialMutex);
        }
        if (id >= 0)
        {
            log_msg(LOG_TRACE, "UartThread", "Received response for request with id: %d", id);
            if (pthread_mutex_lock(&uart_queue_mutex) == 0)
            {
                if(result_type == T_UPDATE_PUSH)
                {
                    log_msg(LOG_TRACE, __cfunc__, "Received T_UPDATE_PUSH with length: %u", bytes_read);
                    if(!JN_update_callback) {
                        log_msg(LOG_ERROR, __cfunc__, "JN_update_callback not initialized!");
                        continue;
                    }
                    JN_update_callback((const char*)local_buffer, bytes_read);
                    pthread_mutex_unlock(&uart_queue_mutex);
                    continue;
                }
                if(uart_queue[id].state != SENT)
                {
                    log_msg(LOG_ERROR, "UartThread", "Received unexpected response with id: %d (current state is: %d). Dropping...", id, uart_queue[id].state);
                    pthread_mutex_unlock(&uart_queue_mutex);
                    continue;
                }

                uint16_t bytes_to_copy = (bytes_read < uart_queue[id].response_buffer_length) ? bytes_read : uart_queue[id].response_buffer_length;

                memcpy(uart_queue[id].response_buffer_address, local_buffer, bytes_to_copy);
                uart_queue[id].is_response_truncated = (bytes_to_copy < bytes_read); 
                uart_queue[id].response_data_length = bytes_to_copy;
                uart_queue[id].request_type = result_type;

                change_state(&uart_queue[id], RECEIVED);

                pthread_mutex_unlock(&uart_queue_mutex);
                log_msg(LOG_TRACE, "UartThread", "Done with mutex");
            }
        }
        msleep(5);
    }

    pthread_exit(NULL);
}

static void change_state(struct uart_queue_item* item, enum uart_queue_item_state state)
{
    if (item->previous_position != -1)
    {
        uart_queue[item->previous_position].next_position = item->next_position;
    }
    else
    {
        first[item->state] = item->next_position;
    }

    if (item->next_position != -1)
    {
        uart_queue[item->next_position].previous_position = item->previous_position;
    }

    item->next_position = first[state];
    item->previous_position = -1;

    if (first[state] != -1)
    {
        uart_queue[first[state]].previous_position = item->request_id; 
    }

    first[state] = item->request_id;
    
    item->state = state; 

    if (state == RECEIVED)
    {
        // we assume that uart_queue_mutex is locked right now!
        pthread_cond_signal(&item->response_ready_condition);
    }
}

