#include "contiki.h"
#include "sys/ctimer.h"
#include "led_control.h"
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>

#define LED_RED (1<<11)
#define LED_WHITE (1<<12)
#define LED_ALL (LED_RED | LED_WHITE)

#define LED_CONTROL_PIN (1<<4)
#define GPIO_POLL_TIME CLOCK_SECOND/10

static uint16_t blink_period;
static struct ctimer blink_timer, gpio_control_timer;
static uint8_t is_led_on;
static uint32_t led_color = LED_WHITE;

static void led_set_color(uint32_t color);

static void led_on(){
	vAHI_DioSetOutput(led_color, (LED_ALL)-led_color);
}

static void led_off(){
	vAHI_DioSetOutput(0, LED_ALL);
}

static void led_gpio_control_callback(void *ptr){
  if(u32AHI_DioReadInput() & LED_CONTROL_PIN){
    if(led_color != LED_WHITE || blink_period != CLOCK_CONF_SECOND/4){
      led_set_color(LED_WHITE);
      led_blink(LED_MODE_BLINK_500);
    }
  }
  else if(led_color != LED_RED || blink_period != CLOCK_CONF_SECOND/4){
    led_set_color(LED_RED);
    led_blink(LED_MODE_BLINK_500);
  }
  ctimer_set(&gpio_control_timer, GPIO_POLL_TIME, led_gpio_control_callback, NULL);
}

void led_stop_polling(){
  ctimer_stop(&gpio_control_timer);
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
    case LED_RED_ON:
        blink_period = 0;
        led_set_color(LED_RED);
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
		case LED_WHITE_ON:
				blink_period = 0;
				led_set_color(LED_WHITE);
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

static void led_set_color(uint32_t color){
	switch(color){
		case LED_RED:
			led_color = LED_RED;
			break;
		case LED_WHITE:
			led_color = LED_WHITE;
			break;
		default:
			led_color = LED_WHITE;
			break;
	}
}

void led_init(){
    vAHI_DioSetDirection(0, LED_RED | LED_WHITE);
    led_blink(LED_MODE_OFF);
    ctimer_set(&gpio_control_timer, GPIO_POLL_TIME, led_gpio_control_callback, NULL);
}
