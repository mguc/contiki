#ifndef _UART_COMMANDS_H
#define _UART_COMMANDS_H

#include "stl.h"
#include "Event.h"
#include "uart_protocol.h"

using namespace ustl;
using namespace guilib;

typedef struct
{
    uint8_t* ssid;
    int ssid_length;
    uint8_t* pass;
    int pass_length;
    uint8_t* gui_loc;
    int gui_loc_length;
    uint8_t* host_ip;
    int host_ip_length;
} zero_config_t;

typedef struct
{
    char* name;
    char* file_name;
    uint32_t version;
    uint32_t crc32;
} firmware_info_t;

enum REST_method
{
    REST_GET = T_REST_GET,
    REST_POST = T_REST_POST,
    REST_PUT = T_REST_PUT,
    REST_DELETE = T_REST_DELETE
};

bool JN_is_firmware_up_to_date(const char * versionPath);

void JN_get_zero_config_button(void * sender, const Event::ArgVector& Args);

int JN_get_brain_info(char * data, uint16_t data_max_length, uint16_t * actual_length);

int JN_get_local_ip(char * data, uint16_t data_max_length, uint16_t * actual_length);

int JN_get_zero_config(const char * hostname, zero_config_t* config);

int JN_get_firmware_info(const char * firmware_name, firmware_info_t* fw_info);

int JN_get_cp6_list(vector<string> *cp6list);

int JN_pair_with_cp6(string address);


int JN_rest_request(REST_method method, string uri, string data, uint8_t* response, uint32_t response_max_length);

#ifdef __cplusplus
extern "C"{
#endif

int JN_check_status(int* state);

#ifdef __cplusplus
}
#endif

#endif
