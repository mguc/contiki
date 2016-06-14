#include <stdio.h>
#include <string.h>
#include "channel_selection.h"

void print_values(unsigned char *buf, int size){
  int i;
  for(i = 0; i < size; i++)
    printf("\t%d", buf[i]);
}

int main(){
  int i, j, channel_count = 0;
  unsigned char channel_buffer[16];
  for(i = 0; i < WIFI_CHANNELS; i++){
    memset(channel_buffer, 0, 16);
    printf("Wifi channel %d: ", wifi_channels[i].id);
    channel_count = get_clear_sixlowpan_channels(wifi_channels[i].center_freq, channel_buffer, 16);
    if(channel_count < 0)
      printf("\nCould not get clear sixlowpan channels, error code: %d", channel_count);
    else{
      printf("\nFound %d clear sixlowpan channels", channel_count);
      print_values(channel_buffer, channel_count);
    }
      
    printf("\n");
  }
  printf("\n");
  
  // should return -1
  printf("TEST: Channel not found\n");
  channel_count = get_clear_sixlowpan_channels(0, channel_buffer, 16);
  if(channel_count < 0)
    printf("Could not get clear sixlowpan channels, error code: %d", channel_count);
  else{
    printf("Found %d clear sixlowpan channels", channel_count);
    print_values(channel_buffer, channel_count);
  }
  printf("\n\n");  
  
  // should return first 4 channels
  printf("TEST: Buffer too small\n");
  channel_count = get_clear_sixlowpan_channels(wifi_channels[0].center_freq, channel_buffer, 4);
  if(channel_count < 0)
    printf("\nCould not get clear sixlowpan channels, error code: %d", channel_count);
  else{
    printf("Found %d clear sixlowpan channels", channel_count);
    print_values(channel_buffer, channel_count);
  }
  printf("\n\n");
  
  return 0;
}
