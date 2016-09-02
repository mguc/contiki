#include "contiki.h"
#include "sys/etimer.h"
#include "cmd.h"
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>
#include "common-defs.h"
#include <stdio.h> /* For printf() */

#define DEBUG 1
#include "net/ip/uip-debug.h"

#define STATE_ZERO_SAMPLE       0
#define STATE_FREQ_DISCOVER     1
#define STATE_SEQUENCE_COLLECT  2
#define STATE_READY             3
#define STATE_NOTIFIED          4

#define IR_MOD_COMB             8
#define IR_MOD_COMB_2			      16
#define IR_MOD_COMB_INT         E_AHI_DIO8_INT
#define IR_LEARN_BUF_MAX_SIZE     (COMMON_BUFFER_SIZE/4)
/*---------------------------------------------------------------------------*/
PROCESS(ir_learn_process, "IR Learn process");
/*---------------------------------------------------------------------------*/
volatile uint32_t last_timestamp;
volatile uint32_t new_period;
volatile uint32_t timer4_val;
static uint32_t i;
static uint32_t* sequence;
/*
0 - '!N\x00\x00'
1 - avg_period
2..2048 - samples
*/
static uint32_t sequence_index;
volatile uint32_t measurements_index;
volatile uint32_t state;
static uint32_t avg_period;
static uint32_t max_samples;

void dio_int_callback(uint32_t u32DeviceId, uint32_t u32ItemBitmap)
{
  uint32_t timestamp;
  if(u32ItemBitmap & IR_MOD_COMB_INT) {
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
        if(sequence_index >= max_samples) {
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

void
ir_learn_init(void)
{
  PRINTF("IR Learn: init (available: %u samples)\n", IR_LEARN_BUF_MAX_SIZE);
  // Configure Timer 4 as Timer
  vAHI_TimerEnable(E_AHI_TIMER_4, 4, FALSE, TRUE, FALSE);
  vAHI_Timer4RegisterCallback((void*) timer4_callback);

  last_timestamp = 0;
  measurements_index = 0;
  sequence_index = 2;
  new_period = 0;
  timer4_val = 0;
  avg_period = 0;
  i = 0;
  state = STATE_ZERO_SAMPLE;
  max_samples = 0;

  vAHI_SysCtrlRegisterCallback(dio_int_callback);
  vAHI_DioSetDirection((1<<IR_MOD_COMB) | (1<<IR_MOD_COMB_2) | (1<<0), 0);
  vAHI_DioInterruptEdge((1<<IR_MOD_COMB), 0);
  vAHI_DioInterruptEnable((1<<IR_MOD_COMB), 0);
  sequence = (uint32_t*)ir_common_buffer;
  PRINTF("IR Learn: init ok\n");
}

uint8_t
ir_learn_start(uint32_t sample_num)
{
  PRINTF("irlearn: start (samples = %lu)\n", sample_num);

  max_samples = sample_num + 2;
  if(max_samples > IR_LEARN_BUF_MAX_SIZE)
    return 1;

  last_timestamp = 0;
  measurements_index = 0;
  sequence_index = 2;
  new_period = 0;
  avg_period = 0;
  state = STATE_ZERO_SAMPLE;

  for(i = 0; i < IR_LEARN_BUF_MAX_SIZE; i++)
    sequence[i] = 0;

  vAHI_TimerStartRepeat(E_AHI_TIMER_4, 0x7fff, 0xffff);
  PRINTF("irlearn: start ok\n");

  return 0;
}

void
ir_learn_send_results(void)
{
  PRINTF("irlearn: send result\n");
  ((char*)sequence)[0] = '!';
  ((char*)sequence)[1] = 'N';
  ((char*)sequence)[2] = 0x20;
  ((char*)sequence)[3] = 0x00;

  sequence[1] = avg_period;

  cmd_send((const unsigned char*)sequence, sizeof(uint32_t) * sequence_index);

  PRINTF("irlearn: send result ok\n");
}

void
ir_learn_stop(void)
{
  PRINTF("irlearn: stop (state = %lu, sequence_index = %lu)\n", state, sequence_index);

  state = STATE_NOTIFIED;

  sequence[2]+=16; //adjust first sequence, we consumed 16 pulses to measuere frequency

  /* DEBUG */
  if((state == STATE_READY) || (state == STATE_NOTIFIED)) {
    // PRINTF("******* SEQUENCE START **************\n");
    // PRINTF("T = %lu us\n", avg_period);
    // for(i = 2; i < sequence_index; i++) {
    //   PRINTF("Sequence[%lu] = %lu\n", i, sequence[i]);
    // }
    // PRINTF("******* SEQUENCE STOP **************\n");
    ir_learn_send_results();
  }
  PRINTF("irlearn: stop ok\n");
}

PROCESS_THREAD(ir_learn_process, ev, data)
{
  char msg[] = "!N\x03\x00";
  PROCESS_BEGIN();
  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_POLL) {
      if(state == STATE_READY) {
        PRINTF("irlearn: process poled, msg send\n");
        cmd_send((const unsigned char*)msg, sizeof(msg));
        state = STATE_NOTIFIED;
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
