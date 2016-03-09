#include "contiki.h"
#include "rtc.h"
#include "sys/clock.h"

/* clock resolution is 1000/125=8ms */
#define CLOCK_RESOLUTION_IN_MS 1000/CLOCK_SECOND

uint32_t rtc_get_raw_ms(void)
{
    return CLOCK_RESOLUTION_IN_MS*clock_time();
}

void rtc_get_time(rtc_t *t)
{
    uint32_t temp;
    temp = CLOCK_RESOLUTION_IN_MS*clock_time();

    t->ms = temp % 1000;
    temp = temp / 1000;
    t->ss = temp % 60;
    temp = temp / 60;
    t->mm = temp % 60;
    temp = temp / 60;
    t->hh = temp % 24;
}
