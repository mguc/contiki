#ifndef _CONTIKI_COMM_H
#define _CONTIKI_COMM_H

#ifdef __cplusplus
extern "C"{
#endif

int32_t jn5168_send(uint8_t type, uint8_t message_id, uint8_t * buffer, int32_t length);
int32_t jn5168_receive(uint8_t * buffer, int32_t max_length, int32_t * buffer_length, int32_t timeout_in_ms, int32_t* result_type);

#ifdef __cplusplus
}
#endif

#define __cfunc__ (char*)__func__
#endif
