#include <stdint.h>
#include "tr2-debug.h"
#include "serial-protocol.h"

msg_t debug_msg;

void debug(char *debug_message){
	debug_msg.type = T_DEBUG;
	debug_msg.data = debug_message;
	debug_msg.len = strlen(debug_message) + 1;
	send_msg(&debug_msg);
}
