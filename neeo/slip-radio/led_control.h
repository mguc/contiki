#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#define LED_MODE_OFF        0
#define LED_MODE_ON         1
#define LED_MODE_BLINK_200  2
#define LED_MODE_BLINK_500  3
#define LED_MODE_BLINK_1000 4

void led_init();
void led_blink(uint8_t mode);

#endif
