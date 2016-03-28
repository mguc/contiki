#include "contiki.h"
#include "sys/ctimer.h"
#include "led_control.h"
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>

#define LED_RED (1<<11)
#define LED_BLUE (1<<12)
#define LED_GREEN (1<<13)

static uint16_t blink_period;
static struct ctimer blink_timer;
static uint8_t is_led_on;

static void led_on(){
	vAHI_DioSetOutput(LED_RED | LED_BLUE | LED_GREEN, 0);
}

static void led_off(){
	vAHI_DioSetOutput(0, LED_RED | LED_BLUE | LED_GREEN);
}

void blink_callback(void *ptr)
{
    if(is_led_on){
    	led_off();
    	is_led_on = 0;
    }
    else{
    	led_on();
    	is_led_on = 1;
    }
    ctimer_set(&blink_timer, blink_period, blink_callback, NULL);
}

void led_blink(uint8_t mode)
{
	ctimer_stop(&blink_timer);
    switch(mode) {
    case LED_MODE_OFF:
        blink_period = 0;
        led_off();
        is_led_on = 0;
        break;
    case LED_MODE_ON:
        blink_period = 0;
        led_on();
        is_led_on = 1;
        break;
    case LED_MODE_BLINK_200:
        blink_period = CLOCK_CONF_SECOND/10;
        led_on();
        is_led_on = 1;
        break;
    case LED_MODE_BLINK_500:
        blink_period = CLOCK_CONF_SECOND/4;
        led_on();
        is_led_on = 1;
        break;
    case LED_MODE_BLINK_1000:
        blink_period = CLOCK_CONF_SECOND/2;
        led_on();
        is_led_on = 1;
        break;
    default:
        blink_period = 0;
        led_off();
        is_led_on = 0;
    	break;
    }
    if(blink_period){
    	ctimer_set(&blink_timer, blink_period, blink_callback, NULL);
    }
}

void led_init(){
    vAHI_DioSetDirection(0, LED_RED | LED_BLUE | LED_GREEN);
    led_blink(LED_MODE_OFF);
}
