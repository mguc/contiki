#include "channel_control.h"
#include "contiki.h"
#include "radio.h"
#include "dev/watchdog.h"

static channel_t channels[OPERATING_CHANNELS];

#define CHANNEL_TIMEOUT CLOCK_SECOND/10
#define CHANNEL_MAX_TRIES 3
#define LOCAL_PORT 3200
#define REMOTE_PORT 3200
#define DISCOVERY_DUTY_CYCLE CLOCK_SECOND/2
#define RSSI_WAIT_TIME RTIMER_SECOND / 20
#define NOISE_MAX_SHIFT 10

static struct etimer discovery_duty_cycle;

static struct simple_udp_connection server_conn;

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

int get_rf_channel(void)
{
  radio_value_t chan;
  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  return (int)chan;
}

int set_rf_channel(radio_value_t ch)
{
  if(ch < FIRST_CHANNEL || ch > LAST_CHANNEL){
    PRINTF("Not a valid channel: %u\n", ch);
    return -1;
  }
  PRINTF("Setting new channel: %u\n", ch);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, ch);
  return 0;
}

/*---------------------------------------------------------------------------*/
static void discover_callback(struct simple_udp_connection *c,
                       const uip_ipaddr_t *source_addr, uint16_t source_port,
                       const uip_ipaddr_t *dest_addr, uint16_t dest_port,
                       const uint8_t *data, uint16_t datalen)
{

  if(strcmp((char*)data, "NBR?") == 0 && datalen == 4)
  {
    uint32_t buf_len = 48;
    char buf[48] = {0};
    print_addr(source_addr, buf, &buf_len);
    PRINTF("Dicovery from: %s on channel %d\n", buf, get_rf_channel());
    PRINTF("I'm NBR!\n");
    buf_len = 48;
    memcpy(buf, "Y ", 2);
    buf_len -= 2;
    print_addr(dest_addr, buf+2, &buf_len);
    c->remote_port = source_port;
    uip_ipaddr_t addr;
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(c, buf, buf_len+2, &addr);
    
  }
  else
    PRINTF("Received unknown response with length %u: %s", datalen, data);
}
/*---------------------------------------------------------------------------*/
void channel_noise_update(channel_t *ch){
  ++ch->quality.noisefloor_latest_sample;
  if(ch->quality.noisefloor_latest_sample >= NOISEFLOOR_SAMPLES){
    ch->quality.noisefloor_latest_sample = 0;
  }
  int index = ch->quality.noisefloor_latest_sample;
  int rssi_level;
  NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &rssi_level);
  ch->quality.noisefloor_samples[index] = rssi_level;
  int i, current_average = 0;
  for(i = 0; i < NOISEFLOOR_SAMPLES; i++)
    current_average += ch->quality.noisefloor_samples[i];
  
  ch->quality.noisefloor_average = current_average/NOISEFLOOR_SAMPLES;
}

void channels_init(channel_t *p_channels, uint32_t number_of_channels){
  int i;
  for(i = 0; i < number_of_channels; i++) {
    p_channels[i].number = (uint8_t)operating_channels[i];
    p_channels[i].quality.status = VERY_GOOD;
    p_channels[i].quality.noisefloor_latest_sample = 0;
    p_channels[i].quality.noisefloor_average = 0;
  }
  for(i = 0; i < NOISEFLOOR_SAMPLES; i++) {
    int j;
    for(j = 0; j < number_of_channels; j++) {
      set_rf_channel(p_channels[j].number);
      rtimer_clock_t wt;
      wt = RTIMER_NOW();
      watchdog_periodic();
      while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + RTIMER_SECOND/20)) {
  #if CONTIKI_TARGET_COOJA
        simProcessRunValue = 1;
        cooja_mt_yield();
  #endif /* CONTIKI_TARGET_COOJA */
      }
      channel_noise_update(&p_channels[j]);
    }
  }
}

void channels_state_print(channel_t *p_channels, uint32_t number_of_channels){
  int i;
  for(i = 0; i < number_of_channels; i++) {
    PRINTF("Channel: %d, Noisefloor latest: %d, Noisefloor average: %d\n", \
      p_channels[i].number, \
      p_channels[i].quality.noisefloor_samples[p_channels[i].quality.noisefloor_latest_sample], \
      p_channels[i].quality.noisefloor_average);
  }
}

static int channels_get_best(channel_t *p_channels, uint32_t number_of_channels){
  int i, best_average = 0, best_channel = 0;
  for(i = 0; i < number_of_channels; i++) {
    if(best_average > p_channels[i].quality.noisefloor_average){
      best_channel = i;
      best_average = p_channels[i].quality.noisefloor_average;
    }
  }
  return best_channel;
}

PROCESS_THREAD(channel_control, ev, data)
{
  static int current_channel_index = 0;
  static int initial_noise_average = 0;
  
  PROCESS_BEGIN();

  channels_init(channels, OPERATING_CHANNELS);
  channels_state_print(channels, OPERATING_CHANNELS);
  current_channel_index = channels_get_best(channels, OPERATING_CHANNELS);
  PRINTF("Best channel: %d\n", channels[current_channel_index].number);
  initial_noise_average = channels[current_channel_index].quality.noisefloor_average;
  set_rf_channel(channels[current_channel_index].number);
  simple_udp_register(&server_conn, LOCAL_PORT, NULL, REMOTE_PORT, discover_callback);
  
  etimer_set(&discovery_duty_cycle, CLOCK_SECOND);
  while(1) {
    
    PROCESS_YIELD();

    switch(ev){
      case PROCESS_EVENT_MSG:
        break;
      case PROCESS_EVENT_TIMER:
        channel_noise_update(&channels[current_channel_index]);
        if(initial_noise_average + NOISE_MAX_SHIFT < channels[current_channel_index].quality.noisefloor_average)
          PRINTF("There might be a better channel! Noise: %d\n", \
            channels[current_channel_index].quality.noisefloor_average);
        etimer_set(&discovery_duty_cycle, CLOCK_SECOND);
        break;
      default:
        break;
    }

  }

  PROCESS_END();
}
