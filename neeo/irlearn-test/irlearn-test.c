#include "contiki.h"
#include "sys/etimer.h"
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>

#include <stdio.h> /* For printf() */

#define STATE_ZERO_SAMPLE       0
#define STATE_FREQ_DISCOVER     1
#define STATE_SEQUENCE_COLLECT  2
#define STATE_READY             3

/*---------------------------------------------------------------------------*/
PROCESS(ir_learn_process, "IR Learn process");
AUTOSTART_PROCESSES(&ir_learn_process);
/*---------------------------------------------------------------------------*/
volatile uint32_t last_timestamp;
volatile uint32_t new_period;
volatile uint32_t timer4_val;
static struct etimer et;
static uint32_t i;
static uint32_t sequence[2048];
static uint32_t sequence_index;
volatile uint32_t measurements_index;
volatile uint32_t state;
static uint32_t avg_period;

void dio_int_callback(uint32_t u32DeviceId, uint32_t u32ItemBitmap)
{
  printf("x\n");
  uint32_t timestamp;
  if(u32ItemBitmap & E_AHI_DIO3_INT) {
    timestamp = timer4_val + u16AHI_TimerReadCount(E_AHI_TIMER_4);
    if(timestamp < last_timestamp) {
      // hack: test proved that the timer4_callback is fired too late. Results:
      // printf("underflow ts=%08lx lts=%08lx\n", timestamp, last_timestamp);
      // underflow ts=0c1e0003 lts=0c1effe8
      // underflow ts=0c420002 lts=0c42ffe7
      // underflow ts=0cda0000 lts=0cdaffe4
      // underflow ts=0d350003 lts=0d35ffe8
      timestamp += 0x10000;
    }

    new_period = timestamp - last_timestamp;
    last_timestamp = timestamp;

    switch(state) {
      case STATE_ZERO_SAMPLE:
        // this stage is required to store the last timestamp
        // and get a proper new_period for frequency detection
        state = STATE_FREQ_DISCOVER;
        break;
      case STATE_FREQ_DISCOVER:
        avg_period += new_period;
        measurements_index++;
        if(measurements_index == 16) {
          avg_period = avg_period / 16;
          state = STATE_SEQUENCE_COLLECT;
        }
        break;
      case STATE_SEQUENCE_COLLECT:
        // we assume a threshold to detect if interrupt comes from 'on' pulse or from end of 'off' pulse
        if(new_period > (2 * avg_period))
        {
          //jump to 'off' part of sequence
          sequence_index++;
          // store the 'off' part
          sequence[sequence_index] = new_period - (avg_period / 2);
          // jump to next 'on'
          sequence_index++;
          //sequence_index &= 0x7ff;
        }
        else
        {
          //still the 'on' part of sequence
          sequence[sequence_index]++;
        }
        if(sequence_index >= 1024) {
          state = STATE_READY;
        }
        break;
      case STATE_READY:
        process_poll(&ir_learn_process);
        break;
      default:
        break;
    }
  }
}
void timer4_callback(uint32_t u32DeviceId, uint32_t u32ItemBitmap)
{
  timer4_val += 0x10000;
  if(timer4_val == 0xffff0000) {
    // overflow of a counter
    timer4_val = (last_timestamp & 0xffff0000);
    last_timestamp &= 0x0000ffff;
  }
}

PROCESS_THREAD(ir_learn_process, ev, data)
{
  PROCESS_BEGIN();
  printf("IR learn!\n");
  
  // Configure Timer 4 as Timer
  vAHI_TimerEnable(E_AHI_TIMER_4, 4, FALSE, TRUE, FALSE);
  vAHI_TimerStartRepeat(E_AHI_TIMER_4, 0x7fff, 0xffff);
  vAHI_Timer4RegisterCallback((void*) timer4_callback);

  last_timestamp = 0;
  measurements_index = 0;
  sequence_index = 0;
  new_period = 0;
  timer4_val = 0;
  avg_period = 0;
  i = 0;
  state = STATE_ZERO_SAMPLE;

  vAHI_SysCtrlRegisterCallback(dio_int_callback);
  vAHI_DioSetDirection((1<<3), (1<<0));
  vAHI_DioInterruptEdge((1<<3), 0);
  vAHI_DioInterruptEnable((1<<3), 0);
  
  etimer_set(&et, CLOCK_SECOND);
  
  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_POLL) {
      if(state == 3) {
        sequence[0]+=16; //adjust first sequence, we consumed 16 pulses to measuere frequency
        printf("******* SEQUENCE START **************\n");
        printf("T = %lu us\n", avg_period);
        for(i = 0; i < sequence_index; i++) {
          printf("Sequence[%lu] = %lu\n", i, sequence[i]);
        }
        printf("******* SEQUENCE STOP **************\n");
        for(i=0; i<2048; i++)
          sequence[i] = 0;
        last_timestamp = 0;
        measurements_index = 0;
        sequence_index = 0;
        new_period = 0;
        avg_period = 0;
        state = STATE_ZERO_SAMPLE;
      }
    }
    else if(ev == PROCESS_EVENT_TIMER) {
      etimer_set(&et, CLOCK_SECOND);
      printf("avg_period = %lu sequence_index = %lu\n", avg_period, sequence_index);
    }
  }  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
