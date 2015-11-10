#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#define COLORS 1
#define LOGGING 1

static void inner_log(const char* source, const char* text, va_list argList, char *type, int color)
{
  char color_s[10];

  if (!LOGGING)
    return;
  
  if (COLORS)
    sprintf(color_s, "\e[1;%dm", color);

  struct tm tm = *localtime(&(time_t){time(NULL)});
  char Buff[32];

  sprintf(Buff, "%s", asctime(&tm));
  Buff[strlen(Buff)-1] = '\0';

  printf("[%s%s%s @ %s] %s%s%s: ", COLORS ? color_s : "", type, COLORS ? "\e[21;39m" : "", Buff, COLORS ? "\e[1;37m" : "", source, COLORS ? "\e[21;39m" : "");
  vprintf(text, argList);
  printf("\n\r");
}

void log_msg(int level, char *type, int color, const char* source, const char* text, ...)
{
  if(level < 0)
    return;

  va_list argList;
  va_start(argList, text);
  inner_log(source, text, argList, type, color);
  va_end(argList);
}

void vlog_msg(int level, char *type, int color, const char* source, const char* text, va_list argList)
{
  if(level < 0)
    return;

  inner_log(source, text, argList, type, color);
}
