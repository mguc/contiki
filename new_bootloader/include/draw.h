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
 * Created on: 30 Oct 2015
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef NEW_BOOTLOADER_INCLUDE_DRAW_H_
#define NEW_BOOTLOADER_INCLUDE_DRAW_H_

#include <unistd.h>
#include <stdint.h>
#include "GUIFont.h"

#define W32(a, b) (*(volatile unsigned int   *)(a)) = b
#define R32(a)    (*(volatile unsigned int   *)(a))

#define PIXEL_FORMAT_A4         10
#define PIXEL_FORMAT_A8         9
#define PIXEL_FORMAT_L4         8
#define PIXEL_FORMAT_AL88       7
#define PIXEL_FORMAT_AL44       6
#define PIXEL_FORMAT_L8         5
#define PIXEL_FORMAT_ARGB4444   4
#define PIXEL_FORMAT_ARGB1555   3
#define PIXEL_FORMAT_RGB565     2
#define PIXEL_FORMAT_RGB888     1
#define PIXEL_FORMAT_ARGB8888   0

#define FOREGROUND_LAYER 1
#define BACKGROUND_LAYER 0

void DrawInit(int width, int height, uint32_t framebufferAddress);

int GetXSize();
int GetYSize();

void SetFremebufferAddress(uint32_t address);
void SetDisplaySize(int32_t x, int32_t y);
void DrawHLine(int32_t Xpos, int32_t Ypos, uint16_t Length, uint32_t text_color);
void DrawVLine(int32_t Xpos, int32_t Ypos, uint16_t Length, uint32_t text_color);
void FillRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t text_color);
void DrawRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t text_color);
void FillCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint32_t color);
void DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint32_t color);
void DrawPixel (uint32_t Xpos, uint32_t Ypos, uint32_t RGB_Code);

void DisplayAntialiasedStringAt(const GUIFont* Font, int16_t Xpos, int16_t Ypos, const char *Text, uint32_t text_color);
void DisplayAntialiasedChar(const GUIFont* Font, uint16_t Xpos, uint16_t Ypos, uint32_t index, uint32_t text_color);
void DrawAntialiasedChar(const GUIFont* Font, int16_t Xpos, int16_t Ypos, const uint8_t* c, uint32_t Index, uint32_t text_color);
void DrawAntialiasedCharInBaund(const GUIFont* Font, int16_t Xpos, int16_t Ypos, int16_t ParentX, int16_t ParentY, int16_t ParentWidth, int16_t ParentHeight, const uint8_t* c, uint32_t Index, uint32_t text_color);

uint32_t GetLayerPixelFormat();
uint32_t GetLayerBytesPerPixel();
uint32_t GetActiveLayerPointer();

#endif /* NEW_BOOTLOADER_INCLUDE_DRAW_H_ */
