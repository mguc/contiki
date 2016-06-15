#ifndef CHANNEL_CONTROL_H
#define CHANNEL_CONTROL_H

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#define FIRST_CHANNEL 11
#define LAST_CHANNEL 26

#define NOISEFLOOR_SAMPLES 8

typedef enum channel_event_e {
  DISCOVER,
  BEST_CHANNEL
} channel_event_t;

typedef enum channel_status_e {
  BAD,
  OK,
  VERY_GOOD
} channel_status_t;

typedef struct channel_quality_s {
  channel_status_t status;
  int noisefloor_latest_sample;
  int noisefloor_average;
  int noisefloor_samples[NOISEFLOOR_SAMPLES];
} channel_quality_t;

typedef struct channel_s {
  uint8_t number;
  channel_quality_t quality;
} channel_t;

typedef struct channel_control_msg_s {
  uint8_t data[16];
  uint32_t len;
} channel_control_msg_t;

int get_rf_channel(void);
int set_rf_channel(radio_value_t ch);

PROCESS_NAME(channel_control);

#endif /* CHANNEL_CONTROL_H */
