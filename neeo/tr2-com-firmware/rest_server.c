#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "rest.h"
#include "rest_server.h"
#include "serial-protocol.h"

#define DEBUG_LEVEL DEBUG_NONE
#include "log_helper.h"

PROCESS(rest_server_process, "REST server process");

RESOURCE(update, METHOD_POST, "update");

msg_t message;

/*---------------------------------------------------------------------------*/
void
update_handler(REQUEST* request, RESPONSE* response)
{
  /* TODO: implement some sanity checks */
  INFOT("REST: Request received!\n");

  message.message_id = 0x00;
  message.message_type = T_UPDATE_PUSH;
  message.data = request->payload;
  message.len = request->payload_len;

  send_msg(&message);
  INFOT("REST: message send, len: %u\n", message.len);

  rest_set_response_status(response, OK_200);
}

PROCESS_THREAD(rest_server_process, ev, data)
{
  PROCESS_BEGIN();
  INFOT("REST: Starting REST server\n");
  rest_init();

  rest_activate_resource(&resource_update);

  PROCESS_END();
}