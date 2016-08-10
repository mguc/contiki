#ifndef IO_POWER_CONTROL_H
#define IO_POWER_CONTROL_H

/*
  This module enables device power control over a DIO pin. On falling edge 
  sleep mode is activated and on rising edge the device returns to its normal
  operation.
*/

// DIO4
#define POWER_CONTROL_PIN (1 << 4)

void io_power_control_init();

#endif /* IO_POWER_CONTROL_H */
