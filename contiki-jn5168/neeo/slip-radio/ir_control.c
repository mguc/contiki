#include "contiki.h"
#include "sys/etimer.h"
#include "ir_control.h"
#include "cmd.h"
#include <string.h>
#include <stdlib.h>
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>

#define DEBUG 1
#include "net/ip/uip-debug.h"

#define SEQ_STATE_READY     0x00
#define SEQ_STATE_PENDING   0x01
#define SEQ_STATE_SENT      0x02

#define MAX_PAYLOAD_LENGTH 2048

#define DEFAULT_REPEAT_COUNT 150

typedef struct {
    uint32_t period;
    uint16_t count;
    uint16_t offset;
    uint16_t sequence_len;
    uint16_t sequence[MAX_PAYLOAD_LENGTH];
    uint8_t state;
} ir_sequence_t;
/*---------------------------------------------------------------------------*/
PROCESS(ir_control_process, "IR Blaster Control Process");
/*---------------------------------------------------------------------------*/
static ir_sequence_t ir_sequence;
volatile static uint8_t currentVal = 0;
volatile static uint16_t sequence_index = 0;
volatile static uint16_t currentRepetition = 0;
volatile static uint32_t currentPeriod = 0;
/*---------------------------------------------------------------------------*/
static inline void
ir_stop_internal(void)
{
    vAHI_TimerStop(E_AHI_TIMER_2);
    vAHI_TimerStop(E_AHI_TIMER_3);
    vAHI_TimerStartSingleShot(E_AHI_TIMER_2, 1, 1);
    ir_sequence.state = SEQ_STATE_SENT;
    process_poll(&ir_control_process);
}
/*---------------------------------------------------------------------------*/
void
timer2_callback(uint32_t u32DeviceId, uint32_t u32ItemBitmap)
{

}
/*---------------------------------------------------------------------------*/
void
timer3_callback(uint32_t u32DeviceId, uint32_t u32ItemBitmap)
{
    if (currentRepetition < ir_sequence.count) {
        if (currentVal==1) {
            // ON sequence
            vAHI_TimerStartRepeat(E_AHI_TIMER_2, ir_sequence.period/2, ir_sequence.period );
        }
        else {
            // OFF sequence
            vAHI_TimerStop(E_AHI_TIMER_2);
            vAHI_TimerStartSingleShot(E_AHI_TIMER_2, 1, 1);
        }

        // TIMER2 generates pulses (or HIGH state), so we can start TIMER3 (multiple of periods) for a time stored in a sequence
        if(currentPeriod == 0)
        {
            currentPeriod = ir_sequence.period * ir_sequence.sequence[sequence_index];
        }

        if(currentPeriod > 0xffff) {
            vAHI_TimerStartSingleShot(E_AHI_TIMER_3, 0, 0xffff);
            currentPeriod -= 0xffff;
        } else {
            vAHI_TimerStartSingleShot(E_AHI_TIMER_3, 0, currentPeriod);
            currentPeriod -= currentPeriod;
        }

        if(currentPeriod)
            return;

        // change ON/OFF sequence
        currentVal = currentVal ^ 0x01;

        sequence_index++;
        if(sequence_index == ir_sequence.sequence_len) {
            currentRepetition++;
            // Continue with offset
            sequence_index = ir_sequence.offset - 1; // because on1 has array index 0
        }
    }
    else {
        // Make sure PWM signal is high
        ir_stop_internal();
     }
}
/*---------------------------------------------------------------------------*/
void
startTransmission(void)
{
    currentVal = 1;
    sequence_index = 0;
    currentRepetition = 0;
    currentPeriod = 0;
    vAHI_TimerEnable(E_AHI_TIMER_3, 0, FALSE, TRUE, FALSE);
    timer3_callback(0, 0);
}
/*---------------------------------------------------------------------------*/
void
ir_init(void)
{
    // Set timer location DO0 and DO1
    vAHI_TimerSetLocation(E_AHI_TIMER_2, FALSE, TRUE);
    // Configure Timer 2 as PWM
    vAHI_TimerEnable(E_AHI_TIMER_2, 0, FALSE, FALSE, TRUE);
    vAHI_TimerConfigureOutputs(E_AHI_TIMER_2, TRUE, TRUE);
    vAHI_TimerStartSingleShot(E_AHI_TIMER_2, 1, 1);
    // vAHI_Timer2RegisterCallback((void*) timer2_callback);
    // Configure Timer 1 to generate interrupts on 0
    vAHI_Timer3RegisterCallback((void*) timer3_callback);
    vAHI_TimerEnable(E_AHI_TIMER_3, 0, FALSE, TRUE, FALSE);
    /* temporary fix to configure the IR Learning pins as input */
    // vAHI_DioSetDirection((1<<3) | (1<<16), 0);
}
/*---------------------------------------------------------------------------*/
int
ir_start(const uint16_t *data, uint32_t len)
{
    if(len > MAX_PAYLOAD_LENGTH) {
        PRINTF("IR:ERR Payload too large! %u\n", len);
        return 1;
    }
    if(ir_sequence.state != SEQ_STATE_READY) {
        PRINTF("IR:ERR IR Blaster busy! %u\n", ir_sequence.state);
        return 2;
    }

    ir_sequence.state = SEQ_STATE_PENDING;
    int ret = 0;
    uint16_t i;
    uint32_t freq = 0;

    freq = data[2];
    freq |= (data[3] << 16);
    //sanity check, if frequency is lower than 20kHz the BRAIN could consume too much current and potentially crash the whole system!
    if(freq < 20000 || freq > 500000) {
        PRINTF("IR:ERR Bad frequency value! (%lu)\n", freq);
        ret = 3;
        goto cleanup;
    }
    ir_sequence.period = 16000000L/freq;

    ir_sequence.count = data[4];
    if(ir_sequence.count > 31) {
        PRINTF("IR:ERR Count greater than 31! (%u)\n", ir_sequence.count);
        ret = 4;
        goto cleanup;
    }
    else if(ir_sequence.count == 0) {
        ir_sequence.count = DEFAULT_REPEAT_COUNT;
    }


    ir_sequence.offset = data[5];
    if((ir_sequence.offset == 0) || (ir_sequence.offset % 2 == 0)) {
        PRINTF("IR:ERR Offset is zero or not an odd number! (%u)\n", ir_sequence.offset);
        ret = 5;
        goto cleanup;
    }

    for(i = 0; i < (len / 2) - 6; i++) {
        ir_sequence.sequence[i] = data[i + 6];
    }

    if(i < 2) {
        PRINTF("IR:ERR Less than 2 data in sequence! Sequence length: %u\n", i);
        ret = 6;
        goto cleanup;
    }

    if(i % 2) {
        PRINTF("IR:ERR Odd number of data in sequence! Sequence length: %u\n", i);
        ret = 7;
        goto cleanup;
    }

    if(ir_sequence.offset > i) {
        PRINTF("IR:ERR Offset greater than sequence length! (%u > %u)\n", ir_sequence.offset, i);
        ret = 8;
        goto cleanup;
    }

    ir_sequence.sequence_len = i;
    startTransmission();
    goto done;

cleanup:
    ir_sequence.state = SEQ_STATE_SENT;

done:
    return ret;
}

int
ir_stop(void)
{
    ir_stop_internal();

    return 0;
}

PROCESS_THREAD(ir_control_process, ev, data)
{
    char msg[] = "!I\x00\x00";

    PROCESS_BEGIN();

    while(1) {
        PROCESS_YIELD();
        if(ev == PROCESS_EVENT_POLL) {
            if(ir_sequence.state == SEQ_STATE_SENT) {
                cmd_send((const unsigned char*)msg, sizeof(msg));
                ir_sequence.state = SEQ_STATE_READY;
            }
        }
    }
    PROCESS_END();
}
