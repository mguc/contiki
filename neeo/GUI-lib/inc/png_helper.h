#ifndef PNG_HELPER_H
#define PNG_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef struct {
    unsigned char *pointer;
    int len;
    int position;
} memory_pointer_t;

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int bytes_per_pixel;
    unsigned int row_bytes;
    unsigned int color_type;
} png_info_t;

void get_png_info(png_info_t *info, unsigned char *data, int len);
void get_png_data(unsigned char *destination, unsigned char *data, int len, int alpha);

#ifdef __cplusplus
};
#endif

#endif
