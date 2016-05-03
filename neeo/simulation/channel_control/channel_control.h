#ifndef CHANNEL_CONTROL_H
#define CHANNEL_CONTROL_H

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#define FIRST_CHANNEL 11
#define LAST_CHANNEL 26

#define PACKETS_PER_CHANNEL 100

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
} channel_quality_t;

typedef struct channel_s {
  uint8_t number;
  channel_quality_t quality;
} channel_t;

const radio_value_t discovery_channels[] = {11, 16, 21, 26};
#define DISCOVERY_CHANNELS_COUNT \
do { \
  sizeof(discovery_channels)/sizeof(radio_value_t); \
} while(0)

PROCESS(channel_control, "Channel control process");

#endif /* CHANNEL_CONTROL_H */
