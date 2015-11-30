#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

int get_content_length(uint16_t *status, uint8_t* response_buffer, uint32_t response_buffer_length);

#endif