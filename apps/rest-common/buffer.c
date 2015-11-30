/*
 * buffer.c
 *
 *  Created on: Oct 19, 2010
 *      Author: dogan
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "buffer.h"
#if TARGET == jn5168
#include "http-common.h"
#endif

#if TARGET != jn5168
uint8_t* data_buffer;
#else
uint8_t data_buffer[HTTP_DATA_BUFF_SIZE];
#endif
uint16_t buffer_size;
uint16_t buffer_index;

void
delete_buffer(void)
{
  if (data_buffer) {
#if TARGET != jn5168
    free(data_buffer);
    data_buffer = NULL;
#endif
    buffer_index = 0;
    buffer_size = 0;
  }
}

uint8_t*
init_buffer(uint16_t size)
{
#if TARGET != jn5168
  delete_buffer();
  data_buffer = (uint8_t*)malloc(size);
#endif
  if (data_buffer) {
    buffer_size = size;
  }
  buffer_index = 0;

  return data_buffer;
}

uint8_t*
allocate_buffer(uint16_t size)
{
  uint8_t* buffer = NULL;
  int rem = 0;
  /*To get rid of alignment problems, always allocate even size*/
  rem = size % 4;
  if (rem) {
    size+=(4-rem);
  }
  if (buffer_index + size < buffer_size) {
    buffer = data_buffer + buffer_index;
    buffer_index += size;
  }

  return buffer;
}

uint8_t*
copy_to_buffer(void* data, uint16_t len)
{
  uint8_t* buffer = allocate_buffer(len);
  if (buffer) {
    memcpy(buffer, data, len);
  }

  return buffer;
}

uint8_t*
copy_text_to_buffer(char* text)
{
  uint8_t* buffer = allocate_buffer(strlen(text) + 1);
  if (buffer) {
    strcpy(buffer, text);
  }

  return buffer;
}
