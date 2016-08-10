#include "io_power_control.h"
#include "net/netstack.h"
#include <AppHardwareApi.h>

static void io_power_control_callback(uint32 u32Device, uint32 u32ItemBitmap) {
  if(u32ItemBitmap & E_AHI_DIO4_INT) {
    // if POWER_CONTROL_PIN is high, the device just woke up, else sleep mode
    // was triggered
    if(u32AHI_DioReadInput() & POWER_CONTROL_PIN){
      // make sure the radio is on again
      NETSTACK_RDC.on();
      // disable wakeup interrupt
      vAHI_DioWakeEnable(0, POWER_CONTROL_PIN);

      // enable the sleep interrupt again
      vAHI_DioInterruptEdge(0, POWER_CONTROL_PIN);
      vAHI_DioInterruptEnable(POWER_CONTROL_PIN, 0);

    }
    else {
      // disable interrupt
      vAHI_DioInterruptEnable(0, POWER_CONTROL_PIN);

      // setup wakeup interrupt
      vAHI_DioWakeEdge(POWER_CONTROL_PIN, 0);
      vAHI_DioWakeEnable(POWER_CONTROL_PIN, 0);

      // goto sleep
      vAHI_Sleep(E_AHI_SLEEP_OSCON_RAMON);
    }
  }
}

// POWER_CONTROL_PIN interrupt configuration to enter sleep mode if triggered
void io_power_control_init() {
  // set pin as input
  vAHI_DioSetDirection(POWER_CONTROL_PIN, 0);
  // remove pull-up
  vAHI_DioSetPullup(0, POWER_CONTROL_PIN);
  // trigger sleep on falling edge
  vAHI_DioInterruptEdge(0, POWER_CONTROL_PIN);
  // register callback
  vAHI_SysCtrlRegisterCallback(io_power_control_callback);
  vAHI_DioInterruptEnable(POWER_CONTROL_PIN, 0);
}
