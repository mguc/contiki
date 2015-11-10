#ifndef _LOG_HELPER_H_
#define _LOG_HELPER_H_

#include <stdio.h>

#define LOG_WARNING 4, "WRN", 95
#define LOG_INFO 3, "INF", 34
#define LOG_TRACE 1, "TRC", 39
#define LOG_DEBUG 2, "DBG", 39
#define LOG_ERROR 5, "ERR", 91
#define LOG_IGNORE -1, "IGN", 39

#ifdef __cplusplus
extern "C" {
#endif
void log_msg(int level, char *type, int colour, char* source, const char* text, ...);
void vlog_msg(int level, char *type, int color, char* source, const char* text, va_list argList);
#ifdef __cplusplus
}
#endif

#define __cfunc__ (char*)__func__

#endif
