#include "contiki.h"
#include <stdlib.h>
#include <string.h>
#include "tr2-common.h"

#define DEBUG_LEVEL DEBUG_NONE
#include "log_helper.h"

static char* trim_backwards(char* beginning, char* end)
{
    while ((*end == ' ' || *end == '\t') && (end != beginning))
    {
        end--;
    }

    return end;
}

static char* find_in_line(char* line, char c)
{
    while (*line != '\r' && *line != '\n' && *line != 0)
    {
        if (*line == c)
        {
            return line;
        }
        line++;
    }

    return NULL;
}

static char* next_line(char* line)
{
    while (*line != '\n' && *line != 0)
    {
        line++;
    }

    if (*line == '\n')
    {
        line++;
    }

    return line;
}

static char* end_of_line(char* line)
{
    if (*line == 0)
    {
        return NULL;
    }

    while (*(line + 1) != '\r' && *(line + 1) != '\n' && *(line + 1) != 0)
    {
        line++;
    }

    return line;
}

static uint32_t line_length(char* string)
{
    uint32_t result = 0;
    while(*string != '\r' && *string != '\n' && *string != '\0')
    {
        result++;
        string++;
    }

    return result;
}

static char* trim_beginning(char* string)
{
    if (string == NULL)
    {
        return NULL;
    }

    while (*string == ' ' || *string == '\t')
    {
        string++;
    }

    return string;
}

static int parse_http_header(char* line, char** name, uint16_t* name_length, char** value, uint16_t* value_length)
{
    // read until ':' which is delimiter of header name
    *name = trim_beginning(line);
    char* end_of_name = find_in_line(*name, ':');
    char* value_start = end_of_name + 1;

    if (end_of_name == NULL)
    {
        ERRORT("HTTP: Error parsing http header: %s\n", line);
        return -1;
    }

    end_of_name = trim_backwards(*name, end_of_name - 1);
    *name_length = (end_of_name - *name);

    *value = trim_beginning(value_start);
    char* end_of_value = end_of_line(*value); 

    if (end_of_value == NULL)
    {
        ERRORT("HTTP: Error parsing http header: %s\n", line);
        return -1;
    }

    end_of_value = trim_backwards(*value, end_of_value);
    *value_length = (end_of_value - *value) + 1;

    return 0;
}

static int parse_status_line(char* status_line, uint16_t* status_code, char** status_phrase, uint32_t* status_phrase_length)
{
    char* sc_pointer = trim_beginning(strstr(status_line, " "));
    if (sc_pointer == NULL)
    {
        ERRORT("HTTP: Error parsing http status line: %s\n", status_line);
        return -1;
    }
    
    *status_code = atoi(sc_pointer);
    if (*status_code < 100 || *status_code > 999)
    {
        ERRORT("HTTP: Error parsing http status code: %s\n", sc_pointer);
        return -1;
    }

    char* sp_pointer = sc_pointer + 3;
    *status_phrase = trim_beginning(sp_pointer);
    *status_phrase_length = line_length(sp_pointer);
    return 0;
}

int get_content_length(uint16_t *status, uint8_t* response_buffer, uint32_t response_buffer_length)
{
    // read HTTP response code
    char* status_phrase;
    uint32_t status_phrase_length;
    int data_length = -1;

    int status_line_parse_result = parse_status_line((char*) response_buffer, status, &status_phrase, &status_phrase_length);
    if (status_line_parse_result != 0)
    {
        ERRORT("HTTP: Error parsing status line.\n");
        return -1;
    }
    INFOT("HTTP: Response received, code: %u, phrase: %s\n", *status, status_phrase);

    char* header_line = next_line((char*)response_buffer);

    // interpret HTTP headers to find length of content
    uint16_t header_name_length;
    uint16_t header_value_length;
    char* header_name;
    char* header_value;

    uint32_t current_line_length = line_length(header_line);

    int parsing_result = parse_http_header(header_line, &header_name, &header_name_length, &header_value, &header_value_length);
    while (parsing_result == 0)
    {
        if (strncmp("Content-Length", header_name, header_name_length) == 0)
        {
            data_length = atoi(header_value);
            break;
        }

        header_line = next_line(header_line);
        current_line_length = line_length(header_line);
        if (current_line_length == 0)
        {
            // end of headers
            data_length = -1;
            break;
        }
        parsing_result = parse_http_header(header_line, &header_name, &header_name_length, &header_value, &header_value_length);
    }

    return data_length;
} 