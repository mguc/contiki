#include <string.h>
#include <stdio.h>

char crc8(const void *vptr, int len)
{
  const char *data = (char*)vptr;
  unsigned crc = 0;
  int i, j;
  for (j = len; j; j--, data++) {
          crc ^= (*data << 8);
          printf("%x\n\r", crc);
          for(i = 8; i; i--) {
                  if (crc & 0x8000)
                    crc ^= (0x1070 << 3);
                  crc <<= 1;
                  printf("%x\n\r", crc);
          }
  }
  return (char)(crc >> 8);
}

int main(){
  char * my_str = "/projects/home/tr2/zero_conf_xml";
  char crc = crc8(my_str, strlen(my_str));
  printf("%s, %x\n\r", my_str, crc & 0x000000FF);
  return 0;
}
