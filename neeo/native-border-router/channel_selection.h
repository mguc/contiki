#ifndef CHANNEL_SELECTION_H
#define CHANNEL_SELECTION_H

typedef struct channel_s {
  unsigned int id;
  unsigned int lower_freq;
  unsigned int center_freq;
  unsigned int upper_freq;
} channel_t;

#define WIFI_CHANNELS 14
#define SIXLOWPAN_CHANNELS 16

extern const channel_t wifi_channels[];
int get_clear_sixlowpan_channels(unsigned int wifi_frequency, unsigned char *channel_buffer, unsigned int max_size);

#endif /* CHANNEL_SELECTION_H */
