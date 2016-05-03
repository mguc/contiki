#include "channel_control.h"
#include "contiki.h"
#include "radio.h"

#define CHANNEL_TIMEOUT CLOCK_SECOND/10
#define LOCAL_PORT 3200
#define REMOTE_PORT 3200
#define DISCOVERY_DUTY_CYCLE CLOCK_SECOND/2

static struct etimer discovery_duty_cycle;
static struct ctimer channel_timeout;
static radio_value_t previous_channel, current_channel;

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

AUTOSTART_PROCESSES(&channel_control);

static void
print_addr(const uip_ipaddr_t *addr, char* buf, uint32_t* len)
{
  char *wr_ptr = buf;
  uint8_t ret;

  if(addr == NULL || addr->u8 == NULL) {
    *len = 0;
    return;
  }
  uint16_t a;
  unsigned int i;
  int f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        strncpy(wr_ptr, "::", 2);
        wr_ptr += 2;
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        strncpy(wr_ptr, ":", 1);
        wr_ptr += 1;
      }
      ret = sprintf(wr_ptr, "%x", a);
      wr_ptr += ret;
    }
  }
  wr_ptr+=1;
  *wr_ptr = 0;
  *len = wr_ptr - buf;
}

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

static void channel_timeout_callback(void *p_msg){
  set_rf_channel(previous_channel);
  uint8_t buf[2] = {'C', (uint8_t)current_channel};
  uip_ipaddr_t addr;
  uip_create_linklocal_allnodes_mcast(&addr);
  simple_udp_sendto((struct simple_udp_connection *)p_msg, buf, 2, &addr);
  set_rf_channel(current_channel);
  ctimer_set(&channel_timeout, CHANNEL_TIMEOUT, channel_timeout_callback, NULL);
}

/*---------------------------------------------------------------------------*/
static void discover_callback(struct simple_udp_connection *c,
                       const uip_ipaddr_t *source_addr, uint16_t source_port,
                       const uip_ipaddr_t *dest_addr, uint16_t dest_port,
                       const uint8_t *data, uint16_t datalen)
{

  if(strcmp((char*)data, "NBR?") == 0 && datalen == 4)
  {
    etimer_stop(&discovery_duty_cycle);
    uint32_t buf_len = 48;
    char buf[48] = {0};
    print_addr(source_addr, buf, &buf_len);
    PRINTF("Dicovery from: %s on channel %d", buf, get_rf_channel());
    PRINTF("I'm NBR!\n");
    buf_len = 48;
    memcpy(buf, "Y ", 2);
    buf_len -= 2;
    print_addr(dest_addr, buf+2, &buf_len);
    c->remote_port = source_port;
    uip_ipaddr_t addr;
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(c, buf, buf_len+2, &addr);
    
    process_post_synch(&channel_control, PROCESS_EVENT_MSG, NULL);
  }
  else if(data[0] == 'C' && datalen == 2){
    PRINTF("Changed successfully to new channel!\n");
    /* TODO: Run channel tests */
    previous_channel = current_channel;
    current_channel++;
    if(current_channel > LAST_CHANNEL){
      PRINTF("Finished.");
    }
    else {
      uint8_t buf[2] = {'C', current_channel};
      uip_ipaddr_t addr;
      uip_create_linklocal_allnodes_mcast(&addr);
      c->remote_port = source_port;
      simple_udp_sendto(c, buf, 2, &addr);
      set_rf_channel(current_channel);
      ctimer_set(&channel_timeout, CHANNEL_TIMEOUT, channel_timeout_callback, NULL);
    }
  }
  else if(data[0] == 'T' && datalen == 2){
    
  }
  else
    PRINTF("Received unknown response with length %u: %s", datalen, data);
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(channel_control, ev, data)
{
  static struct simple_udp_connection server_conn;
  
  static int current_discovery_channel_index = 0;
  static int *p_message;
  
  PROCESS_BEGIN();
  
  set_rf_channel(discovery_channels[current_discovery_channel_index++]);
  simple_udp_register(&server_conn, LOCAL_PORT, NULL, REMOTE_PORT, discover_callback);
  etimer_set(&discovery_duty_cycle, DISCOVERY_DUTY_CYCLE);
  
  while(1) {
    PROCESS_YIELD();

    p_message = (int*) data;
    switch(ev){
      case PROCESS_EVENT_MSG:
        PRINTF("Starting channel stress test\n");
        previous_channel = get_rf_channel();
        current_channel = FIRST_CHANNEL;
        uint8_t buf[2] = {'C', FIRST_CHANNEL};
        uip_ipaddr_t addr;
        uip_create_linklocal_allnodes_mcast(&addr);
        simple_udp_sendto(&server_conn, buf, 2, &addr);
        set_rf_channel(current_channel);
        ctimer_set(&channel_timeout, CHANNEL_TIMEOUT, channel_timeout_callback, &server_conn);
        break;
      case PROCESS_EVENT_TIMER:
        if(current_discovery_channel_index >= sizeof(discovery_channels)/sizeof(radio_value_t))
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
