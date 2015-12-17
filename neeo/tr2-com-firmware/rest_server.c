#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "rest-engine.h"
#include "serial-protocol.h"

#define DEBUG_LEVEL DEBUG_NONE
#include "log_helper.h"

static void update_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

PROCESS(rest_server_process, "REST server process");

resource_t res_update;
RESOURCE(res_update,
         "update",
		 NULL,
		 update_handler,
         NULL,
         NULL);

msg_t message;
uint8_t update_message_buffer[1024] = {0};

/*---------------------------------------------------------------------------*/
void
update_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /* TODO: implement some sanity checks */
  INFOT("REST: Request received!\n");
  message.id = 0x00;
  message.type = T_UPDATE_PUSH;
  message.len = REST.get_request_payload(request, &message.data);
  memcpy(update_message_buffer, message.data, message.len);
  message.data = update_message_buffer;
  send_msg(&message);
  INFOT("REST: message send, len: %u\n", message.len);
}

PROCESS_THREAD(rest_server_process, ev, data)
{
  PROCESS_BEGIN();
  INFOT("REST: Starting REST server\n");
  rest_init_engine();

  rest_activate_resource(&res_update, "update");

  PROCESS_END();
}
