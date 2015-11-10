#ifndef LOG_HELPER_H
#define LOG_HELPER_H


#define DEBUG_NONE      0
#define DEBUG_WARN      1
#define DEBUG_INFO      2
#define DEBUG_ALL       DEBUG_WARN | DEBUG_INFO

void print_timestamp(void);

#define ERROR(...) printf(__VA_ARGS__)
#define ERRORT(...) do { \
                        print_timestamp();   \
                        printf(__VA_ARGS__);  \
                    } while (0)

#if (DEBUG_LEVEL) & DEBUG_WARN
#define WARNING(...) printf(__VA_ARGS__)
#define WARNINGT(...) do { \
                        print_timestamp();   \
                        printf(__VA_ARGS__);  \
                    } while (0)
#else
#define WARNING(...)
#define WARNINGT(...)
#endif

#if (DEBUG_LEVEL) & DEBUG_INFO
#define INFO(...) printf(__VA_ARGS__)
#define INFOT(...) do { \
                        print_timestamp();   \
                        printf(__VA_ARGS__);  \
                    } while (0)
#else
#define INFO(...)
#define INFOT(...)
#endif

#endif