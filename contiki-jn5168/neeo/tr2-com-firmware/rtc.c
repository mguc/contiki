#include "contiki.h"
#include "rtc.h"
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>

volatile static uint32_t ticks;

void timer1_callback(uint32_t u32DeviceId, uint32_t u32ItemBitmap)
{
    ticks++;
}

uint32_t rtc_get_raw_ms(void)
{
    return ticks;
}

void rtc_get_time(rtc_t *t)
{
    uint32_t temp;

    temp = ticks;

    t->ms = temp % 1000;
    temp = temp / 1000;
    t->ss = temp % 60;
    temp = temp / 60;
    t->mm = temp % 60;
    temp = temp / 60;
    t->hh = temp % 24;
}

void rtc_init(uint32_t offset)
{
    ticks = offset;
    vAHI_Timer1RegisterCallback((void*) timer1_callback);
    // Prescaler = 7 -> DIV = 128 -> Timer1 CLK = 125 kHz
    vAHI_TimerEnable(E_AHI_TIMER_1, 7, FALSE, TRUE, FALSE);
    vAHI_TimerStartRepeat(E_AHI_TIMER_1, 60, 125); // 2nd + 3rd parameter should get a number of T1 ticks generate int
}