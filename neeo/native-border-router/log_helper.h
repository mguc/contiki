#ifndef _LOG_HELPER_H_
#define _LOG_HELPER_H_

#include <stdio.h>

#define LOG_WARNING 4, "WRN", 95, __func__
#define LOG_INFO 3, "INF", 34, __func__
#define LOG_TRACE 1, "TRC", 39, __func__
#define LOG_DEBUG 2, "DBG", 39, __func__
#define LOG_ERROR 5, "ERR", 91, __func__
#define LOG_IGNORE -1, "IGN", 39, __func__

#ifdef __cplusplus
extern "C" {
#endif
void log_msg(int level, char *type, int colour, const char* source, const char* text, ...);
void vlog_msg(int level, char *type, int color, const char* source, const char* text, va_list argList);
#ifdef __cplusplus
}
#endif

#endif
