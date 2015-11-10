#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

int bm_init(void);
double bm_read(void);

#ifdef __cplusplus
};
#endif

#endif