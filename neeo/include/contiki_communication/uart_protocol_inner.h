#ifndef _UART_PROTOCOL_INNER_H
#define _UART_PROTOCOL_INNER_H

static void change_state(struct uart_queue_item* item, enum uart_queue_item_state state);

static void *JNUartThread(void* arg);
#endif
