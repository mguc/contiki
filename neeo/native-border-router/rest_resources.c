#include <stdlib.h>
#include <string.h>
#include "rest.h"
#include "border-router.h"
#include "rest_resources.h"
#include "log_helper.h"

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

#define IRBLASTER_START 0x0000
#define IRBLASTER_STOP  0x0001
#define SWAP_BYTES16(__x) ((((__x) & 0xff) << 8) | ((__x) >> 8))
#define SWAP_BYTES32(__x) (((__x) >> 24) | (((__x) & 0x00FF0000) >> 8) | (((__x) & 0x0000FF00) << 8) | ((__x) << 24))
/*---------------------------------------------------------------------------*/
command_state_t sendir_state;
command_state_t learnir_state;
uint32_t irlearn_data[8192];
uint32_t irlearn_data_length;
static char buffer[10*1024];
extern int discover_enabled;
/*---------------------------------------------------------------------------*/
PROCESS(rest_server_process, "Rest Server");
/*---------------------------------------------------------------------------*/
RESOURCE(blink, METHOD_POST, "blink");
RESOURCE(sendir, METHOD_POST | METHOD_GET, "sendir");
RESOURCE(stopir, METHOD_POST, "stopir");
RESOURCE(discovery, METHOD_POST, "discovery");
RESOURCE(learnir, METHOD_POST | METHOD_GET, "learnir");
/*---------------------------------------------------------------------------*/
void
send_stopir(void)
{
  uint16_t send_buf[8];

  strcpy((char*)send_buf, "!I");
  send_buf[1] = SWAP_BYTES16(IRBLASTER_STOP);

  write_to_slip((unsigned char*)send_buf, 4);
  log_msg(LOG_INFO, "sendir: stopir command sent\n");
}
/*---------------------------------------------------------------------------*/
void
blink_handler(REQUEST* request, RESPONSE* response)
{
  char mode[32];
  int success = 1;
  char cmd[4] = "!L\0";

  if (rest_get_post_variable(request, "mode", mode, 10)) {
    if (!strcmp(mode,"white")) {
      cmd[2] = 5;
    } else if(!strcmp(mode,"red")) {
      cmd[2] = 1;
    } else if(!strcmp(mode,"off")) {
      cmd[2] = 0;
    } else if(!strcmp(mode,"200") ) {
      cmd[2] = 2;
    } else if(!strcmp(mode,"500") ) {
      cmd[2] = 3;
    } else if(!strcmp(mode,"1000") ) {
      cmd[2] = 4;
    } else {
      log_msg(LOG_ERROR, "unknown 'mode' value: %s\n", mode);
      success = 0;
    }
  } else {
    log_msg(LOG_ERROR, "'mode' parameter not found!\n");
    success = 0;
  }

  if (!success) {
    rest_set_response_status(response, BAD_REQUEST_400);
    return;
  }

  write_to_slip((unsigned char*)cmd, 3);
  rest_set_response_status(response, OK_200);
}
/*---------------------------------------------------------------------------*/
void
copy_request(REQUEST* source, REQUEST* copy)
{
  *copy = *source;
  copy->payload = NULL;
  copy->payload = malloc(copy->payload_len);
  memcpy(copy->payload, source->payload, copy->payload_len);
}
/*---------------------------------------------------------------------------*/
void
sendir_handler(REQUEST* request, RESPONSE* response)
{
  const int id_len = 6;
  const int freq_len = 7;
  const int count_len = 3;
  const int offset_len = 4;
  const int sequence_len = 1024 * 2 * 6;
  static http_request_t request_copy;

  char id[id_len]; //0 - 65535
  char freq[freq_len]; //20000 - 500000
  uint32_t freq_val = 0;
  char count[count_len]; //0 - 31
  uint16_t count_val = 0;
  char offset[offset_len]; //1 - 255, odd only
  uint16_t offset_val = 0;
  char sequence[sequence_len];
  uint16_t send_buf[10*1024];
  char* p = NULL;
  uint16_t buf;

  if(request->request_type == HTTP_METHOD_GET) {
    rest_set_response_status(response, OK_200);
    response->payload_len = 6;
    if(sendir_state.state == COMMAND_STATE_IDLE)
      response->payload = (unsigned char*)"idle\n";
    else
      response->payload = (unsigned char*)"busy\n";
    return;
  }

  if(sendir_state.state == COMMAND_STATE_PENDING)
  {
    rest_set_response_status(response, SERVICE_UNAVAILABLE_503);
    log_msg(LOG_ERROR, "can't process request: IR busy! Response: 503\n");
    return;
  }

  // else if(sendir_state.state == COMMAND_STATE_PENDING_INFINITELY)
  // {
  //   rest_set_response_status(response, SERVICE_UNAVAILABLE_503);
  //   log_msg(LOG_INFO, "sendir: currently the infinite payload is sending, breaking...\n");
  //   send_stopir();
  //   /* TODO: this state will be overwritten by result from interrupted sendir command, how to avoid this? */
  //   sendir_state.state = COMMAND_STATE_PENDING;
  //   sendir_state.result = -1;
  // }
  else if(sendir_state.state == COMMAND_STATE_IDLE)
  {
    copy_request(request, &request_copy);
    sendir_state.state = COMMAND_STATE_PENDING;
    sendir_state.result = -1;
  }
  else if(sendir_state.state == COMMAND_STATE_ERROR)
  {
    log_msg(LOG_ERROR, "last request returned error: %u\n", sendir_state.result);
    sendir_state.state = COMMAND_STATE_PENDING;
    sendir_state.result = -1;
  }

  if(!rest_get_post_variable(&request_copy, "i", id, id_len)) {
    log_msg(LOG_ERROR, "request error: bad ID! Response: 400\n");
    rest_set_response_status(response, BAD_REQUEST_400);
    sendir_state.state = COMMAND_STATE_IDLE;
    return;
  }

  if(!rest_get_post_variable(&request_copy, "f", freq, freq_len)) {
    log_msg(LOG_ERROR, "request error: bad frequency! Response: 400\n");
    rest_set_response_status(response, BAD_REQUEST_400);
    sendir_state.state = COMMAND_STATE_IDLE;
    return;
  }
  else {
    freq_val = strtol(freq, NULL, 10);
    if(freq_val < 20000 || freq_val > 500000) {
      log_msg(LOG_ERROR, "request error: bad frequency value (%u)! Response: 400\n", freq_val);
      rest_set_response_status(response, BAD_REQUEST_400);
      sendir_state.state = COMMAND_STATE_IDLE;
      return;
    }
  }

  if(!rest_get_post_variable(&request_copy, "c", count, count_len)) {
    // count is optional and can be omitted (assume 1)
    count_val = 1;
    log_msg(LOG_ERROR, "count not found, assuming default value (1)\n");
  }
  else {
    count_val = strtol(count, NULL, 10);
    if(count_val > 31) {
      log_msg(LOG_ERROR, "request error: bad count value (%u)! Response: 400\n", count_val);
      rest_set_response_status(response, BAD_REQUEST_400);
      sendir_state.state = COMMAND_STATE_IDLE;
      return;
    }
    // if(count_val == 0) {
    //   sendir_state.state = COMMAND_STATE_PENDING_INFINITELY;
    // }
  }

  if(!rest_get_post_variable(&request_copy, "o", offset, offset_len)) {
    // offset is optional and default is 1
    offset_val = 1;
    log_msg(LOG_INFO, "offset not found, assuming default value (1)\n");
  }
  else {
    offset_val = strtol(offset, NULL, 10);
    if((offset_val % 2) == 0) {
      log_msg(LOG_ERROR, "request error: offset must be odd value (is %u)! Response: 400\n", offset_val);
      rest_set_response_status(response, BAD_REQUEST_400);
      sendir_state.state = COMMAND_STATE_IDLE;
      return;
    }
  }


  if(!rest_get_post_variable(&request_copy, "s", sequence, sequence_len)) {
    log_msg(LOG_ERROR, "request error: sequence not found! Response: 400\n");
    rest_set_response_status(response, BAD_REQUEST_400);
    sendir_state.state = COMMAND_STATE_IDLE;
    return;
  }

  strcpy((char*)send_buf, "!I");
  send_buf[1] = SWAP_BYTES16(IRBLASTER_START);
  send_buf[2] = SWAP_BYTES16(freq_val & 0x0000ffff);
  send_buf[3] = SWAP_BYTES16(freq_val >> 16);
  send_buf[4] = SWAP_BYTES16(count_val);
  send_buf[5] = SWAP_BYTES16(offset_val);

  int i = 6;
  p = sequence;
  do {
    buf = strtoul(p, &p, 10);
    send_buf[i++] = SWAP_BYTES16(buf);
    if(*p == 0)
      break;
    p++;
  } while(*p);

  if(i < 7)
  {
    log_msg(LOG_ERROR, "request error: sequence too short (length = %u)! Response: 400\n", i - 6);
    rest_set_response_status(response, BAD_REQUEST_400);
    sendir_state.state = COMMAND_STATE_IDLE;
    return;
  }

  if(offset_val > (i - 6)) {
    log_msg(LOG_ERROR, "request error: offset is greater than payload lenght (%u > %u)! Response: 400\n", offset_val, i - 6);
    rest_set_response_status(response, BAD_REQUEST_400);
    sendir_state.state = COMMAND_STATE_IDLE;
    return;
  }

  log_msg(LOG_TRACE, "(%lu) !IF%sC%sO%sS%s\n", sizeof(uint16_t)*i, freq, count, offset, sequence);

  write_to_slip((unsigned char*)send_buf, sizeof(uint16_t)*i);
  rest_set_response_status(response, OK_200);
}
/*---------------------------------------------------------------------------*/
void
stopir_handler(REQUEST* request, RESPONSE* response)
{
  send_stopir();

  rest_set_response_status(response, OK_200);
}
/*---------------------------------------------------------------------------*/
void
learnir_handler(REQUEST* request, RESPONSE* response)
{
  const int operation_len = 8;
  char operation[operation_len];
  const int samples_max_len = 16;
  char samples_max[samples_max_len];
  unsigned char msg[8];
  uint32_t samples = 0;

  msg[0] = '!';
  msg[1] = 'N';
  msg[3] = 0;

  if(request->request_type == HTTP_METHOD_POST)
  {
    if(!rest_get_post_variable(request, "operation", operation, operation_len))
    {
      rest_set_response_status(response, BAD_REQUEST_400);
      log_msg(LOG_ERROR, "'operation' attribute not found!\n");
      return;
    }

    /* Learn IR slip command format
    NBR -> slip-radio
    ! N <cmd> <res> <max_samples1> <max_samples2> <max_samples3> <max_samples4>

    slip-radio -> NBR
    ! N <cmd> <res>

    cmd:
    0x01 start irlearning
    0x02 stop irlearning
    0x03 get results
    0x03 data ready to receive from slip-radio

    */
    if(!strcmp(operation, "start")) {
      if(learnir_state.state != COMMAND_STATE_IDLE) {
        /*
        TODO: what if the current learn is in progress? restart it?
        what if learn process is finished, but data is not received by client?
        */
        rest_set_response_status(response, SERVICE_UNAVAILABLE_503);
        log_msg(LOG_ERROR, "learning in progress (%u)!\n", learnir_state.state);
        return;
      }
      if(!rest_get_post_variable(request, "max_samples", samples_max, samples_max_len))
        samples = 2048;
      else
        samples = strtol(samples_max, NULL, 10);

      log_msg(LOG_INFO, "starting learning (samples = %u)\n", samples);
      learnir_state.state = COMMAND_STATE_PENDING;
      /* TODO: add sending start IR learn code here */
      samples = SWAP_BYTES32(samples);
      msg[2] = 0x01;
      memcpy(msg+4, &samples, sizeof(uint32_t));
      write_to_slip((unsigned char*)msg, sizeof(msg));
    }
    else if(!strcmp(operation, "stop")) {
      if(learnir_state.state != COMMAND_STATE_PENDING) {
        /*
        what if the learn process is finished but data not received? clear data?
        */
        rest_set_response_status(response, SERVICE_UNAVAILABLE_503);
        log_msg(LOG_ERROR, "learning in progress (%u)!\n", learnir_state.state);
        return;
      }
      log_msg(LOG_INFO, "learning stopped\n");
      learnir_state.state = COMMAND_STATE_READY;
      /* TODO: add sending stop learn IR command here
        should the learnir_state.state be changed here?
        maybe in process responsible for receiving data from JN5168?
      */
      msg[2] = 0x02;
      write_to_slip((unsigned char*)msg, sizeof(msg));
    }
    else {
      log_msg(LOG_ERROR, "operation not permitted!\n");
      rest_set_response_status(response, BAD_REQUEST_400);
      return;
    }
  }
  else if(request->request_type == HTTP_METHOD_GET)
  {
    if(learnir_state.state != COMMAND_STATE_READY)
    {
      rest_set_response_status(response, NOT_FOUND_404);
      log_msg(LOG_ERROR, "no data available!\n");
      return;
    }
    learnir_state.state = COMMAND_STATE_IDLE;
    /* TODO: put in the response data the data collected by JN5168 and received by learnir process */
    // msg[2] = 0x02;
    // write_to_slip((unsigned char*)msg, sizeof(msg));
    int j;
    // log_msg(LOG_INFO, "******* SEQUENCE START **************\n");
    irlearn_data[0] = SWAP_BYTES32(irlearn_data[0]);
    float frequency = 1000000.0 / (float)irlearn_data[0];
    uint16_t frequency_int = frequency;

    int offset = 0;

    log_msg(LOG_INFO, "raw = %u frequency = %f (int = %u)\n", irlearn_data[0], frequency, frequency_int);
    offset = sprintf(buffer, "f=%u&s=", frequency_int);

    for(j=1; j<irlearn_data_length; j++){
      irlearn_data[j] = SWAP_BYTES32(irlearn_data[j]);
      if(j%2 == 0)
        irlearn_data[j] = irlearn_data[j] / irlearn_data[0];

      offset += sprintf(buffer + offset, "%u.", irlearn_data[j]);
    }
    buffer[offset - 1] = 0;
    log_msg(LOG_INFO, "data: %s\n", buffer);
    response->payload_len = strlen(buffer);
    response->payload = (unsigned char*)buffer;
    // log_msg(LOG_INFO, "******* SEQUENCE STOP **************\n");
    /*
      uint8_t data[xxx];
      response->payload_len = sizeof(data);
      response->payload = data;
    */
    log_msg(LOG_INFO, "data sent!\n");
    rest_set_response_status(response, OK_200);
  }
  else
  {
    rest_set_response_status(response, NOT_FOUND_404);
  }
}
/*---------------------------------------------------------------------------*/
void
discovery_handler(REQUEST* request, RESPONSE* response)
{
  char state[16];
  int success = 1;

  if (rest_get_post_variable(request, "enabled", state, 16)) {
    if (!strcmp(state,"true")) {
      discover_enabled = 1;
    } else if(!strcmp(state,"false")) {
      discover_enabled = 0;
    } else {
      log_msg(LOG_ERROR, "unknown 'enabled' value: %s\n", state);
      success = 0;
    }
  } else {
    log_msg(LOG_ERROR, "'enabled' parameter not found!\n");
    success = 0;
  }

  if (!success) {
    rest_set_response_status(response, BAD_REQUEST_400);
    return;
  }

  rest_set_response_status(response, OK_200);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rest_server_process, ev, data)
{
  PROCESS_BEGIN();
  log_msg(LOG_INFO, "REST: Starting REST server\n");
  rest_init();

  rest_activate_resource(&resource_blink);
  rest_activate_resource(&resource_sendir);
  rest_activate_resource(&resource_stopir);
  rest_activate_resource(&resource_learnir);
  rest_activate_resource(&resource_discovery);
  sendir_state.state = COMMAND_STATE_IDLE;
  sendir_state.result = -1;
  learnir_state.state = COMMAND_STATE_IDLE;
  learnir_state.result = 0;

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
