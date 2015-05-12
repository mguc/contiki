#ifndef TR2COMMON
#define TR2COMMON

#define DEBUG   1
#undef PRINTF
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

PROCESS_NAME(query_process);
PROCESS_NAME(config_process);

#endif /* TR2COMMON */