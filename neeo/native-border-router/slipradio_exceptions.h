#ifndef SLIPRADIO_EXCEPTIONS_H_
#define SLIPRADIO_EXCEPTIONS_H_

typedef enum {
  E_EXC_BUS_ERROR = 0x02,
  E_EXC_TICK_TIMER = 0x05,
  E_EXC_UNALIGNED_ACCESS = 0x06,
  E_EXC_ILLEGAL_INSTRUCTION = 0x07,
  E_EXC_EXTERNAL_INTERRUPT = 0x08,
  E_EXC_SYSCALL = 0x0C,
  E_EXC_TRAP = 0x0E,
  E_EXC_GENERIC = 0x0F,
  E_EXC_STACK_OVERFLOW = 0x10,
  E_EXC_WATCHDOG = 0xFF // added custom value
} eExceptionType;

void register_slipradio_exception(eExceptionType exception_type);
void handle_slipradio_exception();
void update_channel(unsigned char ch);
unsigned int get_exception_count();

#endif /* SLIPRADIO_EXCEPTIONS_H_ */
