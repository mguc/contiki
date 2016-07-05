#include "slipradio_exceptions.h"
#include "border-router.h"
#include <stdio.h>

/* TODO: These settings should be stored somewhere else, e.g. contiki statistics
  content structures.
*/
static unsigned char current_channel = 0;
static unsigned int slipradio_exceptions_count = 0;

void update_channel(unsigned char ch){
  current_channel = ch;
}

unsigned int get_exception_count(){
  return slipradio_exceptions_count;
}

void handle_slipradio_exception(){
  // restore channel
  unsigned char cmd[3] = {'!', 'C', current_channel };
  write_to_slip((unsigned char*)cmd, 3);

  // set LED to steady white again
  cmd[1] = 'L';
  cmd[2] = 5;
  write_to_slip((unsigned char*)cmd, 3);
}

void register_slipradio_exception(eExceptionType exception_type){
  const char *slipradio_exception;
  ++slipradio_exceptions_count;
  
  switch(exception_type) {
    case E_EXC_BUS_ERROR:
      slipradio_exception = "BUS";
      break;

    case E_EXC_UNALIGNED_ACCESS:
      slipradio_exception = "ALIGN";
      break;

    case E_EXC_ILLEGAL_INSTRUCTION:
      slipradio_exception = "ILLEGAL";
      break;

    case E_EXC_SYSCALL:
      slipradio_exception = "SYSCALL";
      break;

    case E_EXC_TRAP:
      slipradio_exception = "TRAP";
      break;

    case E_EXC_GENERIC:
      slipradio_exception = "GENERIC";
      break;

    case E_EXC_STACK_OVERFLOW:
      slipradio_exception = "STACK";
      break;
      
    case E_EXC_WATCHDOG:
      slipradio_exception = "WATCHDOG";
      break;
      
    default:
      slipradio_exception = "UNKNOWN";
      break;
  }
  printf("%s EXCEPTION occured, exception count: %d\n", slipradio_exception, slipradio_exceptions_count);
}
