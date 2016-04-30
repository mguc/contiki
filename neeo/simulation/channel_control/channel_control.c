#include "channel_control.h"
#include "contiki.h"

#define DISCOVERY_DUTY_CYCLE CLOCK_SECOND/2
static struct etimer discovery_duty_cycle;

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

static uint8_t
get_rf_channel(void)
{
  radio_value_t chan;

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  PRINTF("Current channel: %u", chan);
  return (uint8_t)chan;
}

static int
set_rf_channel(radio_value_t chan)
{
  if(chan < FIRST_CHANNEL || chan > LAST_CHANNEL){
    PRINTF("Not a valid channel: %u", chan);
    return -1;
  }
  PRINTF("Setting new channel: %u", chan)
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, chan);
  return 0;
}

/*---------------------------------------------------------------------------*/
static void discover_callback(struct simple_udp_connection *c,
                       const uip_ipaddr_t *source_addr, uint16_t source_port,
                       const uip_ipaddr_t *dest_addr, uint16_t dest_port,
                       const uint8_t *data, uint16_t datalen)
{

  if(strcmp((char*)data, "NBR?") == 0)
  {
    etimer_stop(&discovery_duty_cycle);  
    uint32_t buf_len = 48;
    char buf[48] = {0};
    print_addr(source_addr, buf, &buf_len);
    PRINTF("Dicovery from: %s ", buf);
    PRINTF("I'm NBR!\n");
    buf_len = 48;
    memcpy(buf, "Y ", 2);
    buf_len -= 2;
    print_addr(&tun_address, buf+2, &buf_len);
    c->remote_port = source_port;
    simple_udp_sendto(c, buf, buf_len+2, source_addr);
  }
  if(strcmp(char*)data, "H") == 0{
    PRINTF("Received heartbeat\n");
    char response = 'Y';
    c->remote_port = source_port;
    simple_udp_sendto(c, &response, 1, source_addr);
  }
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(channel_control, ev, data)
{
  static struct simple_udp_connection server_conn;
  
  static int current_discovery_channel_index = 0;
  
  PROCESS_BEGIN();
  
  simple_udp_register(&server_conn, 3200, NULL, 0, discover_callback);

  PRINTF("Channel control process started\n");

  while(1) {
    
    PROCESS_YIELD();
    
    switch(ev){
      case PROCESS_EVENT_MSG:
        switch(data){
          case DISCOVER:
            int current_discovery_channel_index = 0;
            set_rf_channel(discovery_channels[current_discovery_channel_index++])
            etimer_set(&discovery_duty_cycle, DISCOVERY_DUTY_CYCLE);
            break;
        }
        break;
      case PROCESS_EVENT_TIMER:
        if(current_discovery_channel_index >= DISCOVERY_CHANNELS_COUNT)
          current_discovery_channel_index = 0;
        set_rf_channel(discovery_channels[current_discovery_channel_index++]);
        etimer_set(&discovery_duty_cycle, DISCOVERY_DUTY_CYCLE);  
        break;
      default:
        break;
    }

  }

  PROCESS_END();
}
