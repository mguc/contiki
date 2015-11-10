/********************************************************************
 *
 * CONFIDENTIAL
 *
 * ---
 *
 * (c) 2014-2015 Antmicro Ltd <antmicro.com>
 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains
 * the property of Antmicro Ltd.
 * The intellectual and technical concepts contained herein
 * are proprietary to Antmicro Ltd and are protected by trade secret
 * or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Antmicro Ltd.
 *
 * ---
 *
 * Created on: 2015-04-16
 *     Author: mholenko@antmicro.com
 *
 */

#ifndef GUILIB_H_
#define GUILIB_H_

#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "GUIQueue.h"

using namespace guilib;

typedef struct {
     void (*dma_operation) (uintptr_t inputMem, uintptr_t backgroundMem, uintptr_t outputMem, uint32_t NumberOfLine,
         uint32_t PixelPerLine, uint32_t inOffset, uint32_t backgroundOffset, uint32_t outOffset, uint32_t inPixelFormat, uint32_t backgroundPixelFormat, uint32_t outPixelFormat, uint32_t frontColor);
     void (*dma_fill) (uintptr_t dst, uint32_t xs, uint32_t ys, uint32_t offset, uint32_t color_index, uint32_t pixel_format);
     void (*set_layer_pointer) (uint32_t id, uintptr_t addr);
     void (*set_layer_transparency) (uint32_t id, uint8_t value);
     void (*set_background_colour) (uint32_t colour);
     void *(*malloc) (size_t);
     void (*free) (void*);
     void (*wait_for_first_line) ();
     void (*wait_for_reload) ();
     void (*gui_printf) (const char* text, va_list argList);
} gui_callbacks_t;

namespace guilib {

class GUILib {

public:
    static void Init(gui_callbacks_t *n_callbacks);
    static gui_callbacks_t* Callbacks();
    static uint64_t getTimestamp();
    static void Log(const char* text, ...);
};

}

#endif
