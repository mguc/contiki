#ifndef _MSLEEP_H
#define _MSLEEP_H

#include <sys/time.h>
#define MILLISECONDS 1000000

#define msleep(n) { struct timespec Delay; Delay.tv_sec = 0; Delay.tv_nsec = n * MILLISECONDS; nanosleep(&Delay, NULL); }

#endif
