/////////////////
// log helper

#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#define COLORS 1
#define LOGGING 1
#define SHOW_MILISECONDS 0

pthread_mutex_t log_mutex;
static int log_mutex_initialized = 0;
static void inner_log(char* source, const char* text, va_list argList, char *type, int color)
{
   if (!log_mutex_initialized) {
            log_mutex_initialized = 1;
        pthread_mutex_init(&log_mutex, NULL);
    }
    if (!LOGGING) return;
    char color_s[10];
    if (COLORS) {
            sprintf(color_s, "\e[1;%dm", color);
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int Hour = tv.tv_sec / 3600;
    int Minute = (tv.tv_sec - Hour * 3600) / 60;
#ifdef SHOW_MILISECONDS
    char Buff[14];
    sprintf(Buff, "%02d:%02d:%02d.%03lu", Hour, Minute, (unsigned int) (tv.tv_sec % 60), ((tv.tv_usec/1000)%1000));
#else
    char Buff[9];
    sprintf(Buff, "%02d:%02d:%02d", Hour, Minute, (unsigned int) (tv.tv_sec % 60));
#endif

    pthread_mutex_lock(&log_mutex);
    printf("[%s%s%s @ %s] %s%s%s: ", COLORS ? color_s : "", type, COLORS ? "\e[21;39m" : "", Buff, COLORS ? "\e[1;37m" : "", source, COLORS ? "\e[21;39m" : "");
    vprintf(text, argList);
    printf("\n\r");
    pthread_mutex_unlock(&log_mutex);
}

void log_msg(int level, char *type, int color, char* source, const char* text, ...)
{
    if(level < 0)
    {
        return;
    }
    va_list argList;
    va_start(argList, text);
    inner_log(source, text, argList, type, color);
    va_end(argList);
}

void vlog_msg(int level, char *type, int color, char* source, const char* text, va_list argList)
{
    if(level < 0)
    {
        return;
    }
    inner_log(source, text, argList, type, color);
}

#if 0
unsigned char semihosting_getc() {
  unsigned char result = 0;
  asm (
       "mov r0, r0\n"
       "mov r0, 0x7\n"
       "mov r1, 0x0\n"
       "bkpt 0xab\n"
       "mov r0, r0\n"
      :
      :
      :"r0","r1"
  );
  asm("mov %0, r0" : "=l" (result));
  return result;
}

void semihosting_putc(unsigned char c) {
  asm volatile("mov r0, r0");
  asm ("mov r0, 0x03\n"
       "mov r1, %[msg]\n"
       "bkpt 0xab\n"
      :
      :[msg] "r" (&c)
      :"r0","r1"
  );
  asm volatile("mov r0, r0");
}
#endif


///////////////////////////////////////////////
