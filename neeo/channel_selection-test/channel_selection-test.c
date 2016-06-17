#include <stdio.h>
#include <string.h>
#include "channel_selection.h"

static void print_values(unsigned char *buf, int size){
  int i;
  for(i = 0; i < size; i++)
    printf("\t%d", buf[i]);
}

static void region_test(wifi_region_t wifi_region){
  int i, j, channel_count = 0;
  unsigned char channel_buffer[16];
  for(i = 0; i < WIFI_CHANNELS; i++){
    memset(channel_buffer, 0, 16);
    printf("Wifi channel %d: ", wifi_channels[i].id);
    channel_count = get_clear_sixlowpan_channels(wifi_channels[i].center_freq, wifi_region, channel_buffer, 16);
    if(channel_count < 0)
      printf("\nCould not get clear sixlowpan channels, error code: %d", channel_count);
    else{
      printf("\nFound %d clear sixlowpan channels", channel_count);
      print_values(channel_buffer, channel_count);
    }
      
    printf("\n");
  }
  printf("\n");
}

int main(){
  int channel_count = 0;
  unsigned char channel_buffer[16];

  printf("Region NO_REGION:\n");
  region_test(NO_REGION);
  printf("Region US:\n");
  region_test(US);
  printf("Region EU:\n");
  region_test(EU);
  printf("Region AU:\n");
  region_test(AU);
  
  // should return -1
  printf("TEST: Channel not found\n");
  channel_count = get_clear_sixlowpan_channels(0, US, channel_buffer, 16);
  if(channel_count < 0)
    printf("Could not get clear sixlowpan channels, error code: %d", channel_count);
  else{
    printf("Found %d clear sixlowpan channels", channel_count);
    print_values(channel_buffer, channel_count);
  }
  printf("\n\n");  
  
  // should return first 2 channels
  printf("TEST: Buffer too small\n");
  channel_count = get_clear_sixlowpan_channels(wifi_channels[0].center_freq, US, channel_buffer, 2);
  if(channel_count < 0)
    printf("\nCould not get clear sixlowpan channels, error code: %d", channel_count);
  else{
    printf("Found %d clear sixlowpan channels", channel_count);
    print_values(channel_buffer, channel_count);
  }
  printf("\n\n");
  
  return 0;
}
