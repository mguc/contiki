#include "contiki.h"
#include "sys/etimer.h"
#include "led_control.h"
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>

#define PWM_PERIOD_MAX  250 // Timer0 period is 4us, thus PWM period is 1ms
#define PWM_MAX_DUTY    1000

#define LED_RED (1<<11)
#define LED_WHITE (1<<12)

static volatile int16_t duty;
static volatile int16_t speed;
static uint16_t blink_period;
static struct etimer et;
PROCESS(led_control_process, "LED Control Process");

void timer1_callback(uint32_t u32DeviceId, uint32_t u32ItemBitmap)
{
    int16_t val;

    duty += speed;

    if(duty > PWM_MAX_DUTY)
        duty = PWM_MAX_DUTY;
    if(duty < 0)
        duty = 0;

    val = (PWM_PERIOD_MAX * duty) / PWM_MAX_DUTY;
    vAHI_TimerStartRepeat(E_AHI_TIMER_1, val, PWM_PERIOD_MAX);
}

void led_blink(uint8_t mode)
{
    int16_t val;
    switch(mode) {
    case LED_MODE_OFF:
        blink_period = 10;
        speed = 0;
        duty = 99;
        val = (PWM_PERIOD_MAX * duty) / PWM_MAX_DUTY;
        vAHI_TimerDisable(E_AHI_TIMER_1);
        vAHI_DioSetDirection(0, LED_RED | LED_WHITE);
        vAHI_DioSetOutput(0, LED_RED | LED_WHITE);
        break;
    case LED_WHITE_ON:
        blink_period = 10;
        speed = 0;
        duty = 1;
        val = (PWM_PERIOD_MAX * duty) / PWM_MAX_DUTY;
        vAHI_TimerDisable(E_AHI_TIMER_1);
        vAHI_DioSetDirection(0, LED_WHITE);
        vAHI_DioSetOutput(LED_WHITE, 0);
        break;
    case LED_RED_ON:
        blink_period = 10;
        speed = 0;
        duty = 1;
        val = (PWM_PERIOD_MAX * duty) / PWM_MAX_DUTY;
        vAHI_TimerDisable(E_AHI_TIMER_1);
        vAHI_DioSetDirection(0, LED_RED);
        vAHI_DioSetOutput(LED_RED, 0);
        break;
    case LED_MODE_BLINK_200:
        blink_period = 10;
        speed = 10; /* on/off phase is 100ms, so we need to reach 1000 duty in 100ms with 1ms step */
        duty = 0;
        vAHI_TimerDIOControl(E_AHI_TIMER_1, TRUE );
        vAHI_Timer1RegisterCallback((void*) timer1_callback);
        // Prescaler = 6 -> DIV = 64 -> Timer1 CLK = 250 kHz
        vAHI_TimerEnable(E_AHI_TIMER_1, 6, FALSE, TRUE, TRUE);
        val = (PWM_PERIOD_MAX * duty) / PWM_MAX_DUTY;
        vAHI_TimerStartRepeat(E_AHI_TIMER_1, val, PWM_PERIOD_MAX);
        break;
    case LED_MODE_BLINK_500:
        blink_period = 25;
        speed = 4;
        duty = 0;
        vAHI_TimerDIOControl(E_AHI_TIMER_1, TRUE );
        vAHI_Timer1RegisterCallback((void*) timer1_callback);
        // Prescaler = 6 -> DIV = 64 -> Timer1 CLK = 250 kHz
        vAHI_TimerEnable(E_AHI_TIMER_1, 6, FALSE, TRUE, TRUE);
        val = (PWM_PERIOD_MAX * duty) / PWM_MAX_DUTY;
        vAHI_TimerStartRepeat(E_AHI_TIMER_1, val, PWM_PERIOD_MAX);
        break;
    case LED_MODE_BLINK_1000:
        blink_period = 50;
        speed = 2;
        duty = 0;
        vAHI_TimerDIOControl(E_AHI_TIMER_1, TRUE );
        vAHI_Timer1RegisterCallback((void*) timer1_callback);
        // Prescaler = 6 -> DIV = 64 -> Timer1 CLK = 250 kHz
        vAHI_TimerEnable(E_AHI_TIMER_1, 6, FALSE, TRUE, TRUE);
        val = (PWM_PERIOD_MAX * duty) / PWM_MAX_DUTY;
        vAHI_TimerStartRepeat(E_AHI_TIMER_1, val, PWM_PERIOD_MAX);
        break;
    }
}

PROCESS_THREAD(led_control_process, ev, data)
{
    PROCESS_BEGIN();
    etimer_set(&et, blink_period);
    while(1)
    {
        PROCESS_YIELD();
        if(ev == PROCESS_EVENT_TIMER) {
            etimer_set(&et, blink_period);
            speed = -speed;
        }
    }
    PROCESS_END();
}
