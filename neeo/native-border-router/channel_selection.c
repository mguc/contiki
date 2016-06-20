#include <stdio.h>
#include "channel_selection.h"

//Wifi Channel bandwith is 20MHz or 40Mhz (unsure).
//NOTE: the best performance for WIFI is when WIFI channels are at least 5 channels away fron another WIFI channel
const channel_t wifi_channels[WIFI_CHANNELS] = {
  {1, 2390, 2412, 2434},
  {2, 2395, 2417, 2439},
  {3, 2400, 2422, 2444},
  {4, 2405, 2427, 2449},
  {5, 2410, 2432, 2454},
  {6, 2415, 2437, 2459},
  {7, 2420, 2442, 2464},
  {8, 2425, 2447, 2469},
  {9, 2430, 2452, 2474},
  {10, 2435, 2457, 2479},
  {11, 2440, 2462, 2484},
  {12, 2445, 2467, 2489},
  {13, 2450, 2472, 2494},
  {14, 2462, 2484, 2506}
};

const channel_t sixlowpan_channels[SIXLOWPAN_CHANNELS] = {
  {11, 2404, 2405, 2406},
  {12, 2409, 2410, 2411},
  {13, 2414, 2415, 2416},
  {14, 2419, 2420, 2421},
  {15, 2424, 2425, 2426},
  {16, 2429, 2430, 2431},
  {17, 2434, 2435, 2436},
  {18, 2439, 2440, 2441},
  {19, 2444, 2445, 2446},
  {20, 2449, 2450, 2451},
  {21, 2454, 2455, 2456},
  {22, 2459, 2460, 2461},
  {23, 2464, 2465, 2466},
  {24, 2469, 2470, 2471},
  {25, 2474, 2475, 2476},
  {26, 2479, 2480, 2481}
};

#define US_PREFERRED_CHANNELS 3
const unsigned int us_preferred_channels[US_PREFERRED_CHANNELS] = { 15, 20, 25 };

#define EU_PREFERRED_CHANNELS 4
const unsigned int eu_preferred_channels[EU_PREFERRED_CHANNELS] = { 15, 16, 21, 22};

#define AU_PREFERRED_CHANNELS 3
const unsigned int au_preferred_channels[US_PREFERRED_CHANNELS] = { 15, 20, 25 };

static int is_channel_preferred(wifi_region_t wifi_region, unsigned int channel_id){
  int i, region_channels;
  const unsigned int *preferred_channels;
  switch (wifi_region) {
    case US:
      region_channels = US_PREFERRED_CHANNELS;
      preferred_channels = us_preferred_channels;
      break;
    case EU:
      region_channels = EU_PREFERRED_CHANNELS;
      preferred_channels = eu_preferred_channels;
      break;
    case AU:
      region_channels = AU_PREFERRED_CHANNELS;
      preferred_channels = au_preferred_channels;
      break;
    default:
      // skip region check
      return 1;
  }
  for(i = 0; i < region_channels; i++){
    if(preferred_channels[i] == channel_id)
      return 1;
  }
  return 0;
}

int get_wifi_channel_id(unsigned int wifi_frequency){
  int i;
  /* Find the wifi channel in the LUT */
  for(i = 0; i < WIFI_CHANNELS; i++){
    if(wifi_channels[i].center_freq == wifi_frequency)
      break;
  }
  if(i == WIFI_CHANNELS){
    printf("Wifi channel not found!\n");
    return -1;
  }
  else
    return wifi_channels[i].id;
}

int get_clear_sixlowpan_channels(unsigned int wifi_frequency, wifi_region_t wifi_region, \
  unsigned char *channel_buffer, unsigned int max_size){
  int i,j, size = 0;
  /* Find the wifi channel in the LUT */
  for(i = 0; i < WIFI_CHANNELS; i++){
    if(wifi_channels[i].center_freq == wifi_frequency)
      break;
  }
  if(i == WIFI_CHANNELS){
    printf("Wifi channel not found!\n");
    return -1;
  }
  /* Exclude channel 26 because tx power is reduced on this channel */
  for(j = 0; j < SIXLOWPAN_CHANNELS - 1; j++){
    if(sixlowpan_channels[j].lower_freq > wifi_channels[i].upper_freq || \
      sixlowpan_channels[j].upper_freq < wifi_channels[i].lower_freq){
        // filter preferred channels in a certain region
        if(is_channel_preferred(wifi_region, sixlowpan_channels[j].id)){
          if(size < max_size)
            channel_buffer[size++] = (unsigned char) sixlowpan_channels[j].id;
          else {
            printf("Warning: Buffer full, returning %d valid channels.\n", size);
            break;
          }
        }
      }
  }
  return size;
}
