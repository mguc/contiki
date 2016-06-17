#include "channel_control.h"
#include "contiki.h"
#include "radio.h"
#include "dev/watchdog.h"

#define DEBUG DEBUG_FULL
#include "net/ip/uip-debug.h"

#define NOISE_DETECTION_TIME CLOCK_SECOND/2
#define CHANNEL_SETTLE_TIME CLOCK_SECOND/10
#define NOISE_SAMPLE_TIME CLOCK_SECOND/1000

PROCESS(channel_control, "Channel control process");
PROCESS(channel_noise_detection, "Channel noise detection process");

static channel_t channels[16];
static int detect_noise = 0;

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
void channel_noise_update(channel_t *ch)
{
  int rssi_level;
  NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &rssi_level);
  ++ch->quality.noisefloor_samples_count;
  ch->quality.noisefloor_latest_sample = rssi_level;
  ch->quality.noisefloor_samples_sum += rssi_level;
  ch->quality.noisefloor_average = ch->quality.noisefloor_samples_sum/ch->quality.noisefloor_samples_count;
}

void channels_init(channel_t *p_channels, uint8_t *p_operating_channels, uint32_t number_of_channels)
{
  int i;
  for(i = 0; i < number_of_channels; i++) {
    p_channels[i].id = p_operating_channels[i];
    p_channels[i].quality.status = VERY_GOOD;
    p_channels[i].quality.noisefloor_latest_sample = 0;
    p_channels[i].quality.noisefloor_average = 0;
    p_channels[i].quality.noisefloor_samples_sum = 0;
    p_channels[i].quality.noisefloor_samples_count = 0;
  }
}

void channels_state_print(channel_t *p_channels, uint32_t number_of_channels)
{
  int i;
  for(i = 0; i < number_of_channels; i++) {
    PRINTF("Channel: %d, latest sample: %d, samples count: %d, average: %d\n", \
      p_channels[i].id, \
      p_channels[i].quality.noisefloor_latest_sample, \
      p_channels[i].quality.noisefloor_samples_count, \
      p_channels[i].quality.noisefloor_average);
  }
}

static int channels_get_best(channel_t *p_channels, uint32_t number_of_channels)
{
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
  static struct etimer et;
  static int current_channel_index = 0;
  static channel_t *channel_ptr = NULL;
  static uint32_t channel_count = 0;

  PROCESS_BEGIN();
  process_start(&channel_noise_detection, NULL);

  while(1) {
    PROCESS_YIELD();

    /* Don't use switch/case statements here, because some macros like
      PROCESS_YIELD(), PROCESS_YIELD_UNTIL() etc. can NOT be used then. */
    if(ev == PROCESS_EVENT_MSG) {
      channel_control_msg_t *msg_ptr = (channel_control_msg_t *)data;
      channel_count = msg_ptr->len;
      channel_ptr = channels;
      PRINTF("Init %ld channels\n", channel_count);
      channels_init(channel_ptr, msg_ptr->data, channel_count);

      detect_noise = 0;
      current_channel_index = 0;
      PRINTF("Setting new channel %d and waiting for %dms\n", \
        channel_ptr[current_channel_index].id, 1000*CHANNEL_SETTLE_TIME/CLOCK_SECOND);
      set_rf_channel(channel_ptr[current_channel_index].id);
      etimer_set(&et, CHANNEL_SETTLE_TIME);
    }
    else if(ev == PROCESS_EVENT_TIMER) {
      if(detect_noise){
        PRINTF("Stopping noise detection on channel %d\n", channel_ptr[current_channel_index].id);
        detect_noise = 0;
        PROCESS_YIELD();
        PRINTF("Noise detection stopped on channel %d\n", channel_ptr[current_channel_index].id);
        ++current_channel_index;
        if(current_channel_index < channel_count){
          PRINTF("Setting new channel %d and waiting for %dms\n", \
            channel_ptr[current_channel_index].id, 1000*CHANNEL_SETTLE_TIME/CLOCK_SECOND);
          set_rf_channel(channel_ptr[current_channel_index].id);
          etimer_set(&et, CHANNEL_SETTLE_TIME);
        }
        else {
          PRINTF("Noisefloor statistics\n");
          channels_state_print(channel_ptr, channel_count);
          int best_channel_index = channels_get_best(channel_ptr, channel_count);
          PRINTF("Best channel: %d\n", channel_ptr[best_channel_index].id);
          set_rf_channel(channel_ptr[best_channel_index].id);
        }
      }
      else{
        PRINTF("Trigger noise detection on channel %d for %dms\n", \
          channel_ptr[current_channel_index].id, 1000*NOISE_DETECTION_TIME/CLOCK_SECOND);
        detect_noise = 1;
        process_post(&channel_noise_detection, PROCESS_EVENT_MSG, &channel_ptr[current_channel_index]);
        etimer_set(&et, NOISE_DETECTION_TIME);
      }
    }
  }

  PROCESS_END();

}

PROCESS_THREAD(channel_noise_detection, ev, data)
{
  static struct etimer et;
  static channel_t *channel_ptr = NULL;

  PROCESS_BEGIN();

  while(1){
    PROCESS_YIELD();

    if(ev == PROCESS_EVENT_MSG) {
      channel_ptr = (channel_t *)data;
      PRINTF("Starting noise detection on channel %d\n", channel_ptr->id);
      etimer_set(&et, 0);
    }
    else if(ev == PROCESS_EVENT_TIMER) {
      channel_noise_update(channel_ptr);
      if(detect_noise){
        etimer_set(&et, NOISE_SAMPLE_TIME);
      }
      else{
        PRINTF("Noise detection on channel %d completed\n", channel_ptr->id);
        process_poll(&channel_control);
      }
    }
  }

  PROCESS_END();

}
