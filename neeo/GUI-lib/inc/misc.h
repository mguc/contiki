#ifndef __MISC_H__
#define __MISC_H__
#include <stdint.h>

#define PIXEL_FORMAT_A4         10
#define PIXEL_FORMAT_A8         9
#define PIXEL_FORMAT_L4         8
#define PIXEL_FORMAT_AL88       7
#define PIXEL_FORMAT_AL44       6
#define PIXEL_FORMAT_L8         5
#define PIXEL_FORMAT_ARGB4444   4
#define PIXEL_FORMAT_ARGB1555   3
#define PIXEL_FORMAT_RGB565  2
#define PIXEL_FORMAT_RGB888 1
#define PIXEL_FORMAT_ARGB8888 0

#define FOREGROUND_LAYER 1
#define BACKGROUND_LAYER 0

extern "C" inline uint32_t pixelFormatToBPP(uint32_t pf) {
        switch(pf)
        {
                case PIXEL_FORMAT_RGB888:
                        return 3;
                case PIXEL_FORMAT_ARGB8888:
                        return 4;
		case PIXEL_FORMAT_A8:
			return 1;
                case PIXEL_FORMAT_RGB565:
                case PIXEL_FORMAT_ARGB4444:
                default:
                        return 2;
        }
}

#endif
