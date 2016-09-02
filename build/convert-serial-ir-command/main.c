#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SWAP_BYTES16(__x) ((((__x) & 0xff) << 8) | ((__x) >> 8))
#define IRBLASTER_START 0x0000

void sendir_handler(uint32_t freq_val, uint16_t offset_val, char sequence[12288]) {
  uint16_t count_val = 1;
  uint16_t send_buf[10*1024];
  char* p = NULL;
  uint16_t buf;

  //example call i=0&f=36000&c=10&o=3&s=21.40.21.40.10.50.10.50'

  strcpy((char*)send_buf, "!I");
  send_buf[1] = SWAP_BYTES16(IRBLASTER_START);
  send_buf[2] = SWAP_BYTES16(freq_val & 0x0000ffff);
  send_buf[3] = SWAP_BYTES16(freq_val >> 16);
  send_buf[4] = SWAP_BYTES16(count_val);
  send_buf[5] = SWAP_BYTES16(offset_val);

  int i = 6;
  p = sequence;
  do {
    buf = strtoul(p, &p, 10);
    send_buf[i++] = SWAP_BYTES16(buf);
    if(*p == 0)
      break;
    p++;
  } while(*p);

  if(i < 7) {
    printf("sequnce too short!\n");
    return;
  }

  if(offset_val > (i - 6)) {
    printf("request error: offset is greater than payload lenght (%u > %u)! Response: 400!\n", offset_val, i - 6);
    return;
  }

  uint8_t *pnt = &(send_buf[0]);
  for (int n = 0; n < i*2; n++) {
    printf("0x%.2X, ", pnt[n]);
  }
  //write footer
  printf("0x%.2X", 0xc0);
}

int main(int argc, const char * argv[]) {
    printf("IR to UART MARSHALLER!\n");
    //sendir,1:1,1,37825,1,1,171,171,21,64,21,64,21,64,21,21,21,21,21,21,21,21,21,21,21,64,21,64,21,64,21,21,21,21,21,21,21,21,21,21,21,64,21,64,21,64,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,64,21,64,21,64,21,64,21,64,21,1776
    printf("samsung tv volume up\n");
    sendir_handler(37825, 1, "171.171.21.64.21.64.21.64.21.21.21.21.21.21.21.21.21.21.21.64.21.64.21.64.21.21.21.21.21.21.21.21.21.21.21.64.21.64.21.64.21.21.21.21.21.21.21.21.21.21.21.21.21.21.21.21.21.64.21.64.21.64.21.64.21.64.21.1776");

    printf("\n\nblt30khz\n");
    //sendir,1:1,1,30000,1,1,0,0,2000,2000,0,0,2000,2000,0,0,2000,2000,0,0,2000,3000 - duration 560ms
    sendir_handler(30000, 1, "0.0.2000.2000.0.0.2000.2000.0.0.2000.2000.0.0.2000.3000");

    printf("\n\nblt500khz\n");
    //sendir,1:1,1,490000,1,1,0,0,16000,16000,0,0,16000,16000,0,0,16000,16000,0,0,16000,17000 - duration 263ms
    sendir_handler(490000, 1, "0.0.16000.16000.0.0.16000.16000.0.0.16000.16000.0.0.16000.17000");

    printf("\n\n");
    return 0;
}
