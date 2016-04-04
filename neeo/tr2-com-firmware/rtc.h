#ifndef RTC_H
#define RTC_H

typedef struct rtc_s {
    uint32_t hh;
    uint32_t mm;
    uint32_t ss;
    uint32_t ms;
} rtc_t;

uint32_t rtc_get_raw_ms(void);
void rtc_get_time(rtc_t *t);

#endif
