#include "channel_control.h"
#include "contiki.h"
#include "radio.h"
#include "dev/watchdog.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define OPERATING_CHANNELS 4
const radio_value_t operating_channels[OPERATING_CHANNELS] = {11, 16, 21, 26};
static channel_t channels[OPERATING_CHANNELS];

#define RSSI_WAIT_TIME (RTIMER_SECOND / 20)
#define NOISE_MAX_SHIFT 10

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
      while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + RSSI_WAIT_TIME)) {
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

PROCESS(channel_control, "Channel control process");
PROCESS_THREAD(channel_control, ev, data)
{
  // static struct etimer et;
  static int current_channel_index = 0;
  static int initial_noise_average = 0;

  PROCESS_BEGIN();

  channels_init(channels, OPERATING_CHANNELS);
  channels_state_print(channels, OPERATING_CHANNELS);
  current_channel_index = channels_get_best(channels, OPERATING_CHANNELS);
  printf("Best channel: %d\n", channels[current_channel_index].number);
  initial_noise_average = channels[current_channel_index].quality.noisefloor_average;
  set_rf_channel(channels[current_channel_index].number);

  // etimer_set(&et, CLOCK_SECOND);
  // while(1) {
  // 
  //   PROCESS_YIELD();
  // 
  //   switch(ev){
  //     case PROCESS_EVENT_MSG:
  //       break;
  //     case PROCESS_EVENT_TIMER:
  //       channel_noise_update(&channels[current_channel_index]);
  //       if(initial_noise_average + NOISE_MAX_SHIFT < channels[current_channel_index].quality.noisefloor_average)
  //         PRINTF("There might be a better channel! Noise: %d\n", \
  //           channels[current_channel_index].quality.noisefloor_average);
  //       etimer_set(&et, CLOCK_SECOND);
  //       break;
  //     default:
  //       break;
  //   }
  // 
  // }

  PROCESS_END();
}
