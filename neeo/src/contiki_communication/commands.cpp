#include "contiki_communication/uart_protocol.h"
#include "contiki_communication/commands.h"
#include "log_helper.h"
#include <string.h>
#include "stl.h"
#include "tinyxml2.h"

using namespace ustl;
using namespace tinyxml2;

#define BUFFER_LENGTH   16
bool JN_is_firmware_up_to_date(const char * firmwareFile)
{
    char actualVersion[BUFFER_LENGTH];
    uint16_t actualLength;
    bool result = true;
    uint32_t versionFromBinary;
    uint32_t versionFromJN;

    FILE * versionFile = fopen(firmwareFile, "rb");
    
    if(versionFile == NULL)
    {
        log_msg(LOG_WARNING, __cfunc__, "Could not open firmware file %s.", firmwareFile);
        return true; //we do not have info about available contiki, so we don't update 
    }

    if(fread(&versionFromBinary, sizeof(uint32_t), 1, versionFile) == 1)
    {
        log_msg(LOG_TRACE, __cfunc__, "Version of firmware in romfs: %d", versionFromBinary);
        int comm_result = send_and_receive(T_JN_VERSION, NULL, 0, (uint8_t *)actualVersion, BUFFER_LENGTH, &actualLength);
        if(comm_result != 0 || actualLength == 0)
        {
            log_msg(LOG_INFO, __cfunc__, "Could not obtain JN5168 firmware version, assuming it's too old.");
            result = false;
        }
        else
        {
            versionFromJN = strtol(actualVersion, NULL, 10);
            log_msg(LOG_INFO, __cfunc__, "Current JN5168 firmware version is %u.", versionFromJN);
            result = (versionFromJN < versionFromBinary) ? false : true;
        }
    }
    else
    {
        result = true; //without proper version info we fail
    }
    fclose(versionFile);
    log_msg(LOG_TRACE, __cfunc__, "Will%s update firmware.", result ? " not" : "");
    return result;
}

int JN_get_zero_config(const char * hostname, zero_config_t* conf)
{
    uint8_t response_buffer[360];
    uint16_t actual_length;
    log_msg(LOG_INFO, __cfunc__, "Asking for WiFi credentials.");
    
    actual_length =  JN_rest_request(REST_GET, "/config/zero_conf.xml", "", response_buffer, 360);
    
    if(actual_length > 0)
    {
        log_msg(LOG_TRACE, __cfunc__, "Received credentials, len %u.", actual_length);
        XMLDocument doc( true, COLLAPSE_WHITESPACE );

        XMLError error = doc.Parse((const char*)(response_buffer), actual_length);
        if(error == XML_SUCCESS)
        {
            XMLElement * root = doc.FirstChildElement("config");
            if(root != 0)
            {
                XMLElement * item = root->FirstChildElement("wifi")->FirstChildElement("ssid");
                if(conf->ssid)
                    free(conf->ssid);
                conf->ssid = (uint8_t*)malloc(strlen(item->GetText()) + 1);
                strcpy((char*)conf->ssid, item->GetText());
                conf->ssid_length = strlen((char*)conf->ssid);

                item = root->FirstChildElement("wifi")->FirstChildElement("password");
                if(conf->pass)
                    free(conf->pass);
                conf->pass = (uint8_t*)malloc(strlen(item->GetText()) + 1);
                strcpy((char*)conf->pass, item->GetText());
                conf->pass_length = strlen((char*)conf->pass);

                item = root->FirstChildElement("gui_loc");
                if(conf->gui_loc)
                    free(conf->gui_loc);
                conf->gui_loc = (uint8_t*)malloc(strlen(item->GetText()) + 1);
                strcpy((char*)conf->gui_loc, item->GetText());
                conf->gui_loc_length = strlen((char*)conf->gui_loc);

                item = root->FirstChildElement("brain_ip");
                if(conf->host_ip)
                    free(conf->host_ip);
                conf->host_ip = (uint8_t*)malloc(strlen(item->GetText()) + 1);
                strcpy((char*)conf->host_ip, item->GetText());
                conf->host_ip_length = strlen((char*)conf->host_ip);

                log_msg(LOG_TRACE, __cfunc__, "Credentials found %s/%s. GUI file at %s/%s", conf->ssid, conf->pass, conf->host_ip, conf->gui_loc);
                doc.Clear();
                return 0;
            }
            else
            {
                log_msg(LOG_ERROR, __cfunc__, "Error in response document.");
            }
        }
        else
        {
            log_msg(LOG_ERROR, __cfunc__, "Error in parsing response.");
        }
        doc.Clear();
    }

    log_msg(LOG_INFO, __cfunc__, "WiFi credentials not received.");

    return -1;
}

int JN_get_firmware_info(const char * firmware_name, firmware_info_t* fw_info)
{
    const uint8_t requestBufferSize = 64;
    char request[requestBufferSize];
    int ret;
    uint8_t response_buffer[360];
    uint16_t actual_length;
    
    log_msg(LOG_INFO, __cfunc__, "Asking for %s firmware info.", firmware_name);
    
    
    ret = snprintf(request, 64, "/update/%s", firmware_name);
    if(ret < 0 || ret >= requestBufferSize)
    {
        log_msg(LOG_ERROR, __cfunc__, "Request URI truncated! Result: %s, desirable: /update/", request, firmware_name);
        return -1;
    }
    
    actual_length =  JN_rest_request(REST_GET, request, "", response_buffer, 360);

    if(actual_length > 0)
    {
        log_msg(LOG_TRACE, __cfunc__, "Received firmware info, len %u.", actual_length);
        XMLDocument doc(true, COLLAPSE_WHITESPACE);
        string XMLFile((const char*)response_buffer);
        uint32_t file_offset = XMLFile.find("?>",0);
        if(file_offset == string::npos) file_offset = 0;
        else file_offset +=2;

        XMLError error = doc.Parse((const char*)(response_buffer + file_offset), actual_length - file_offset);
        if(error == XML_SUCCESS)
        {
            XMLElement * root = doc.FirstChildElement("firmware");
            if(root != 0)
            {
                XMLElement * item = root->FirstChildElement("name");
                if(fw_info->name)
                    free(fw_info->name);
                fw_info->name = (char*)malloc(strlen(item->GetText()) + 1);
                strcpy((char*)fw_info->name, item->GetText());

                item = root->FirstChildElement("file");
                if(fw_info->file_name)
                    free(fw_info->file_name);
                fw_info->file_name = (char*)malloc(strlen(item->GetText()) + 1);
                strcpy((char*)fw_info->file_name, item->GetText());

                item = root->FirstChildElement("version");
                fw_info->version = strtol(item->GetText(), NULL, 10);
                // if(item->QueryUnsignedText(&fw_info->version) != XML_SUCCESS)
                //     log_msg(LOG_ERROR, __cfunc__, "Getting version failed (received: %s)", item->GetText());

                item = root->FirstChildElement("crc32");
                // if(item->QueryUnsignedText(&fw_info->crc32) != XML_SUCCESS)
                //     log_msg(LOG_ERROR, __cfunc__, "Getting crc32 failed (received: %s)", item->GetText());
                fw_info->crc32 = strtoll(item->GetText(), NULL, 16);

                doc.Clear();
                return 0;
            }
            else
            {
                log_msg(LOG_ERROR, __cfunc__, "Error in response document.");
            }
        }
        else
        {
            log_msg(LOG_ERROR, __cfunc__, "Error in parsing response.");
        }
        doc.Clear();
    }

    log_msg(LOG_INFO, __cfunc__, "Firmware info not received.");

    return -1;
}

int JN_get_cp6_list(vector<string> *cp6list)
{
    uint8_t response_buffer[360];
    uint16_t actual_length;

    log_msg(LOG_TRACE, __cfunc__, "Asking to pair with brain.");
    if(send_and_receive(T_GET_CP6_LIST, NULL, 0, response_buffer, 360, &actual_length) != 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error getting CP-6 list");
        return -1;
    }
    
    char * pch;
    pch = strtok ((char*)response_buffer,"\n");
    
    while (pch != NULL)
    {
        string temp(pch);

        cp6list->push_back(temp);
        pch = strtok(NULL, "\n");
    }
    return 0;
}

int JN_pair_with_cp6(string address)
{
    uint8_t address_buffer[48];

    strcpy((char*)address_buffer, address.c_str());
    
    //+1 = null terminating byte
    return send_and_receive(T_PAIR_WITH_CP6, address_buffer, address.size() + 1, NULL, 0, NULL);
}

int JN_get_brain_info(char * data, uint16_t data_max_length, uint16_t * actual_length)
{
    return send_and_receive(T_BRAIN_INFO, NULL, 0, (uint8_t *)data, data_max_length, actual_length);
}

int JN_get_local_ip(char * data, uint16_t data_max_length, uint16_t * actual_length)
{
    return send_and_receive(T_GET_LOCAL_IP, NULL, 0, (uint8_t *)data, data_max_length, actual_length);
}

int JN_check_status(int* state)
{
    uint8_t response_buffer[360];
    uint16_t actual_length;
    // log_msg(LOG_TRACE, __cfunc__, "Asking for jn5168 status.");
    int result = send_and_receive(T_STATUS, NULL, 0, response_buffer, 360, &actual_length);
    if(result == 0)
    {
        *state = response_buffer[0]-'0';
    }
    return result;
}

/*
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
        log_msg(LOG_ERROR, __cfunc__, "Error parsing http header: %s", line);
        return -1;
    }

    end_of_name = trim_backwards(*name, end_of_name - 1);
    *name_length = (end_of_name - *name);

    *value = trim_beginning(value_start);
    char* end_of_value = end_of_line(*value); 

    if (end_of_value == NULL)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error parsing http header: %s", line);
        return -1;
    }

    end_of_value = trim_backwards(*value, end_of_value);
    *value_length = (end_of_value - *value) + 1;

    return 0;
}

static int parse_status_line(char* status_line, uint32_t* status_code, char** status_phrase, uint32_t* status_phrase_length)
{
    char* sc_pointer = trim_beginning(strstr(status_line, " "));
    if (sc_pointer == NULL)
    {
        log_msg(LOG_TRACE, __cfunc__, "Error parsing http status line: %s", status_line);
        return -1;
    }
    
    *status_code = atoi(sc_pointer);
    if (*status_code < 100 || *status_code > 999)
    {
        log_msg(LOG_TRACE, __cfunc__, "Error parsing http status code: %s", sc_pointer);
        return -1;
    }

    char* sp_pointer = sc_pointer + 3;
    *status_phrase = trim_beginning(sp_pointer);
    *status_phrase_length = line_length(sp_pointer);
    return 0;
}

int JN_send_rest_request(string host, string path, string data, string method, uint8_t* response, uint32_t response_max_length)
{
    uint8_t request_buffer[360];
    uint8_t response_buffer[1024];
    int request_length = 0;
    // TODO: change `localhost` to proper brain server ip or hostname
    if(data.length() != 0)
        request_length = snprintf((char*)request_buffer, 360, "%s %s HTTP/1.1\r\nHost: %s\r\nContent-Length: %u\r\n\r\n%s", method.c_str(), path.c_str(), host.c_str(), data.length(), data.c_str());
    else
        request_length = snprintf((char*)request_buffer, 360, "%s %s HTTP/1.1\r\nHost: %s\r\n\r\n", method.c_str(), path.c_str(), host.c_str());

    uint16_t response_length;
    int32_t data_length = -1;

    log_msg(LOG_TRACE, __cfunc__, "Sending %s request to server:\r\n%s", method.c_str(),  request_buffer);
    if (send_and_receive(T_REST_REQUEST, request_buffer, request_length, response_buffer, 1024, &response_length) != 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error while talking to the webserver.");
        return -1;
    }

    // read HTTP response code
    uint32_t status_code;
    char* status_phrase;
    uint32_t status_phrase_length;

    int status_line_parse_result = parse_status_line((char*) response_buffer, &status_code, &status_phrase, &status_phrase_length);
    if (status_line_parse_result != 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error parsing status line.");
        return -1;
    }
    log_msg(LOG_TRACE, __cfunc__, "Response received, code: %d, phrase: %.*s", status_code, status_phrase_length, status_phrase);
    if (status_code != 200)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error status code.");
        return -1;
    }

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
        }

        header_line = next_line(header_line);
        current_line_length = line_length(header_line);
        if (current_line_length == 0)
        {
            // end of headers
            break;
        }
        parsing_result = parse_http_header(header_line, &header_name, &header_name_length, &header_value, &header_value_length);
    }

    if (data_length == -1)
    {
        return -1;
    }

    if (response_max_length < (uint16_t)data_length)
    {
        data_length = response_max_length;
    }

    header_line = next_line(header_line);
    memcpy(response, header_line, data_length);
    return data_length;
}
*/

int JN_rest_request(REST_method method, string uri, string data, uint8_t* response, uint32_t response_max_length)
{
    uint8_t response_buffer[1024];
    uint8_t request_buffer[360];
    uint16_t response_length = 0;
    int request_length = 0;

    memset(request_buffer, 0, sizeof(request_buffer));
    memset(response_buffer, 0, sizeof(response_buffer));

    if(data.length() != 0)
        request_length = snprintf((char*)request_buffer, 360, "%s\n%s", uri.c_str(), data.c_str());
    else
        request_length = snprintf((char*)request_buffer, 360, "%s", uri.c_str());

    if(send_and_receive(method, request_buffer, request_length, response_buffer, 1024, &response_length) != 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error while talking to the webserver.");
        return -1;
    }

    if (response_max_length < (uint16_t)response_length) {
        log_msg(LOG_WARNING, __cfunc__, "Response buffer too short. Received %u bytes, available space is %u bytes.", response_length, response_max_length);
        response_length = response_max_length;
    }

    if(response) {
        memcpy(response, response_buffer, response_length);
    }

    return response_length;
} 
