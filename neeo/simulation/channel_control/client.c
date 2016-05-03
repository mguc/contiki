#include "channel_control.h"
#include "contiki.h"
#include "radio.h"

#define LOCAL_PORT 3200
#define REMOTE_PORT 3200
#define DISCOVERY_DUTY_CYCLE CLOCK_SECOND/10
static struct etimer discovery_duty_cycle;

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

AUTOSTART_PROCESSES(&channel_control);

static int
get_rf_channel(void)
{
  radio_value_t chan;
  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  return (int)chan;
}

static int
set_rf_channel(radio_value_t chan)
{
  if(chan < FIRST_CHANNEL || chan > LAST_CHANNEL){
    PRINTF("Not a valid channel: %u\n", chan);
    return -1;
  }
  PRINTF("Setting new channel: %u\n", chan);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, chan);
  return 0;
}

/*---------------------------------------------------------------------------*/
static void discover_callback(struct simple_udp_connection *c,
                       const uip_ipaddr_t *source_addr, uint16_t source_port,
                       const uip_ipaddr_t *dest_addr, uint16_t dest_port,
                       const uint8_t *data, uint16_t datalen)
{
  if(datalen > 2){
    if(data[0] == 'Y' && data[1] == ' ') {
      etimer_stop(&discovery_duty_cycle);
      PRINTF("Received discovery response on channel: %u\n", get_rf_channel());
      PRINTF("Message was: %s\n", data);
      PRINTF("Staying on this channel and waiting for orders\n");
    }
    else if(data[0] == 'C' && datalen == 2){
      set_rf_channel((radio_value_t)data[1]);
      uip_ipaddr_t addr;
      uip_create_linklocal_allnodes_mcast(&addr);
      c->remote_port = source_port;
      simple_udp_sendto(c, data, datalen, &addr);
    }
    else
      PRINTF("Received unknown response with length %u: %s", datalen, data);
  }
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(channel_control, ev, data)
{
  static struct simple_udp_connection server_conn;
  
  static int current_discovery_channel_index = 0;
  static int *p_message;
  static uip_ipaddr_t addr;
  static uint8_t buf[4]; 
  PROCESS_BEGIN();
  
  simple_udp_register(&server_conn, LOCAL_PORT, NULL, REMOTE_PORT, discover_callback);
  etimer_set(&discovery_duty_cycle, DISCOVERY_DUTY_CYCLE);
  
  while(1) {
    PROCESS_YIELD();

    p_message = (int*) data;
    switch(ev){
      case PROCESS_EVENT_TIMER:
        if(current_discovery_channel_index >= sizeof(discovery_channels)/sizeof(radio_value_t))
          current_discovery_channel_index = 0;
        
        set_rf_channel(discovery_channels[current_discovery_channel_index++]);
        uip_create_linklocal_allnodes_mcast(&addr);
        memcpy(buf, "NBR?", 4);
        PRINTF("Sending discovery message on channel: %u\n", get_rf_channel());
        simple_udp_sendto(&server_conn, buf, 4, &addr);
        etimer_set(&discovery_duty_cycle, DISCOVERY_DUTY_CYCLE);
        break;
      default:
        break;
    }

  }

  PROCESS_END();
}
