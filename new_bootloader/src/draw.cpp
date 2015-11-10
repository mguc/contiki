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

#include "draw.h"
#include "dma2d.h"
#include <semaphore.h>
#include <pkgconf/posix.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/var_io.h>

sem_t DMA2D_Sem;
cyg_handle_t DMA2DIntHandle;
cyg_interrupt DMA2DInt;
int XSize = 0;
int YSize = 0;

static uint32_t framebufferAddress;

static uint32_t convertColor(uint32_t color, uint32_t inputPixelFormat, uint32_t outputPixelFormat);
static void DMA_MoveFont(uintptr_t font_src, uintptr_t font_dst, int32_t x_src, int32_t y_src, int32_t x_dst, int32_t y_dst, int32_t width, int32_t height, int32_t fontWidth, int32_t fontHeight, uint32_t outPixelFormat, uint32_t fontColor);
static void DMA_Fill(uintptr_t outputMem, uint32_t PixelsPerLine, uint32_t NumberOfLines, uint32_t outOffset, uint32_t color_index, uint32_t pixel_format);
static void DMA_Operation(uintptr_t inputMem, uintptr_t backgroundMem, uintptr_t outputMem, uint32_t PixelsPerLine, uint32_t NumberOfLines,
        uint32_t inOffset, uint32_t backgroundOffset, uint32_t outOffset, uint32_t inPixelFormat, uint32_t backgroundPixelFormat, uint32_t outPixelFormat, uint32_t frontColor);

cyg_uint32 DMA2D_ISR(cyg_vector_t vector, cyg_addrword_t data) {
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);
    uint32_t val = R32(_DMA2D_OFFSET + _DMA2D_CR);
    W32(_DMA2D_OFFSET + _DMA2D_CR, val & ~_DMA2D_CR_TCIE);
    W32(_DMA2D_OFFSET + _DMA2D_ISR, _DMA2D_ISR_TCIF);
    return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;
}

void DMA2D_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data) {
    sem_post(&DMA2D_Sem);
    cyg_interrupt_unmask(vector);
}

void DrawInit(int width, int height, uint32_t framebufferAddress) {
    sem_init(&DMA2D_Sem, 0, 0);
    hal_stm32_clock_enable(CYGHWR_HAL_STM32_CLOCK(AHB1, DMA2D));
    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_DMA2D, 0, 0, DMA2D_ISR, DMA2D_DSR, &DMA2DIntHandle, &DMA2DInt);
    cyg_interrupt_configure(CYGNUM_HAL_INTERRUPT_DMA2D, 1, true);
    cyg_interrupt_attach(DMA2DIntHandle);
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_DMA2D);
    SetFremebufferAddress(framebufferAddress);
    SetDisplaySize(width, height);
}

void SetFremebufferAddress(uint32_t address) {
    framebufferAddress = address;
}

int GetXSize() {
    return XSize;
}

int GetYSize() {
    return YSize;
}

void SetDisplaySize(int32_t x, int32_t y) {
    XSize = x;
    YSize = y;
}

void DrawHLine(int32_t Xpos, int32_t Ypos, uint16_t Length, uint32_t text_color) {
    uintptr_t ax = 0;
    uintptr_t ptr = GetActiveLayerPointer();
    uint32_t bytes = GetLayerBytesPerPixel();
    uint32_t pixel_format = GetLayerPixelFormat();
    // sanity checks
    if(Ypos > (int32_t)GetYSize()) return;
    if(Ypos < 0) return;
    if(Xpos < 0) {
        Length = Length + Xpos;
        Xpos = 0;
    }
    if((Xpos + Length) > GetXSize()) {
        Length = GetXSize() - Xpos;
    }

    ax = ptr + bytes * (GetXSize() * Ypos + Xpos);
    DMA_Fill(ax, Length, 1, 0, text_color, pixel_format);
}

void DrawVLine(int32_t Xpos, int32_t Ypos, uint16_t Length, uint32_t text_color) {
    uintptr_t ax = 0;
    uintptr_t ptr = GetActiveLayerPointer();
    uint32_t bytes = GetLayerBytesPerPixel();
    uint32_t pixel_format = GetLayerPixelFormat();
    if(Xpos < 0) return;
    if(Xpos > (int32_t)GetXSize()) return;
    if(Ypos < 0) {
        Length = Length + Ypos;
        Ypos = 0;
    }

    if((Ypos + Length) > GetYSize()) {
        Length = GetYSize() - Ypos;
    }

    ax = ptr + bytes * (GetXSize() * Ypos + Xpos);
    DMA_Fill(ax, 1, Length, (GetXSize() - 1), text_color, pixel_format);
}

uint32_t GetActiveLayerPointer() {
    return framebufferAddress;
}

uint32_t GetLayerBytesPerPixel() {
    return 2;
}

uint32_t GetLayerPixelFormat() {
    return PIXEL_FORMAT_ARGB4444;
}

static void DMA_Fill(uintptr_t outputMem, uint32_t PixelsPerLine, uint32_t NumberOfLines, uint32_t outOffset, uint32_t color_index, uint32_t pixel_format) {
    dma2d_dma_fill(outputMem, PixelsPerLine, NumberOfLines, outOffset, color_index, pixel_format);
    sem_wait(&DMA2D_Sem);
}

static void DMA_Operation(uintptr_t inputMem, uintptr_t backgroundMem, uintptr_t outputMem, uint32_t PixelsPerLine, uint32_t NumberOfLines, uint32_t inOffset, uint32_t backgroundOffset, uint32_t outOffset, uint32_t inPixelFormat, uint32_t backgroundPixelFormat, uint32_t outPixelFormat, uint32_t frontColor) {
    if((PixelsPerLine == 0) || (NumberOfLines == 0) || (PixelsPerLine > 1024) || (NumberOfLines > 1024) || (outOffset > 0x3FFF)) {
        return;
    }
    dma2d_dma_operation(inputMem, backgroundMem, outputMem, PixelsPerLine, NumberOfLines, inOffset, backgroundOffset,
    outOffset, inPixelFormat, backgroundPixelFormat, outPixelFormat, frontColor);
    sem_wait(&DMA2D_Sem);
}

void FillRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t text_color) {
    uintptr_t ax = 0;
    uint32_t bytes = GetLayerBytesPerPixel();
    uintptr_t ptr = GetActiveLayerPointer();
    uint32_t pixel_format = GetLayerPixelFormat();

    ax = ptr + bytes * (GetXSize() * Ypos + Xpos);
    DMA_Fill(ax, Width, Height, (GetXSize() - Width), text_color, pixel_format);
}

void DrawRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t text_color) {
    DrawHLine(Xpos, Ypos, Width-1, text_color);
    DrawHLine(Xpos, (Ypos + Height-1), Width-1, text_color);
    DrawVLine(Xpos, Ypos, Height, text_color);
    DrawVLine((Xpos + Width-1), Ypos, Height, text_color);
}

void FillCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint32_t color) {
    int32_t D; /* Decision Variable */
    uint32_t CurX; /* Current X Value */
    uint32_t CurY; /* Current Y Value */

    D = 3 - (Radius << 1);

    CurX = 0;
    CurY = Radius;
    while (CurX <= CurY) {
        if(CurY > 0) {
            DrawHLine(Xpos - CurY, Ypos + CurX, 2 * CurY, color);
            DrawHLine(Xpos - CurY, Ypos - CurX, 2 * CurY, color);
        }

        if(CurX > 0) {
            DrawHLine(Xpos - CurX, Ypos - CurY, 2 * CurX, color);
            DrawHLine(Xpos - CurX, Ypos + CurY, 2 * CurX, color);
        }

        if(D < 0) {
            D += (CurX << 2) + 6;
        } else {
            D += ((CurX - CurY) << 2) + 10;
            CurY--;
        }
        CurX++;
    }
    DrawCircle(Xpos, Ypos, Radius, color);
}

void DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius, uint32_t color) {
    int32_t D; /* Decision Variable */
    uint32_t CurX; /* Current X Value */
    uint32_t CurY; /* Current Y Value */

    D = 3 - (Radius << 1);
    CurX = 0;
    CurY = Radius;
    while (CurX <= CurY) {
        DrawPixel((Xpos + CurX), (Ypos - CurY), color);
        DrawPixel((Xpos - CurX), (Ypos - CurY), color);
        DrawPixel((Xpos + CurY), (Ypos - CurX), color);
        DrawPixel((Xpos - CurY), (Ypos - CurX), color);
        DrawPixel((Xpos + CurX), (Ypos + CurY), color);
        DrawPixel((Xpos - CurX), (Ypos + CurY), color);
        DrawPixel((Xpos + CurY), (Ypos + CurX), color);
        DrawPixel((Xpos - CurY), (Ypos + CurX), color);

        if(D < 0) {
            D += (CurX << 2) + 6;
        } else {
            D += ((CurX - CurY) << 2) + 10;
            CurY--;
        }
        CurX++;
    }
}

void DrawPixel(uint32_t Xpos, uint32_t Ypos, uint32_t RGB_Code) {
    uintptr_t ptr = GetActiveLayerPointer();
    if(Xpos > (uint32_t) GetXSize()) return;
    if(Ypos > (uint32_t)GetYSize()) return;
    if(GetLayerBytesPerPixel() == 4) {
        uint32_t *pixels = (uint32_t*)ptr;
        pixels[Ypos * GetXSize() + Xpos] = RGB_Code;
    } else {
        uint16_t *pixels = (uint16_t*)ptr;
        uint16_t col = convertColor(RGB_Code, PIXEL_FORMAT_ARGB8888, GetLayerPixelFormat()) & 0xFFFF;
        pixels[Ypos * GetXSize() + Xpos] = col;
    }
}

static uint32_t convertColor(uint32_t color, uint32_t inputPixelFormat, uint32_t outputPixelFormat) {
    uint8_t color1 = 0;
    uint8_t color2 = 0;
    uint8_t color3 = 0;
    uint8_t alpha = 0;
    if(inputPixelFormat == outputPixelFormat) return color;
    switch (inputPixelFormat) {
    case PIXEL_FORMAT_ARGB8888:
        switch (outputPixelFormat) {
        case PIXEL_FORMAT_ARGB4444: //ARGB8888 to ARGB4444
            color1 = (color & 0x00ff0000) >> 20;
            color2 = (color & 0x0000ff00) >> 12;
            color3 = (color & 0x000000ff) >> 4;
            alpha = (color & 0xff000000) >> 28;
            return (uint16_t)((alpha << 12) | (color1 << 8) | (color2 << 4) | (color3));

        case PIXEL_FORMAT_RGB565: //ARGB8888 to RGB565
            color1 = (color & 0x00ff0000) >> 19;
            color2 = (color & 0x0000ff00) >> 10;
            color3 = (color & 0x000000ff) >> 3;
            return (uint16_t)((color1 << 11) | (color2 << 5) | (color3));

        case PIXEL_FORMAT_RGB888: //ARGB8888 to RGB888
            return (color & 0x00FFFFFF);
        }
        break;

    case PIXEL_FORMAT_ARGB4444:
        switch (outputPixelFormat) {
        case PIXEL_FORMAT_ARGB8888: //ARGB4444 to ARGB8888
            color1 = (color & 0x0f00) >> 8;
            color2 = (color & 0x00f0) >> 4;
            color3 = (color & 0x000f) >> 0;
            alpha = (color & 0xf000) >> 12;
            return (uint32_t)((alpha << 28 | alpha << 24) | (color1 << 20 | color1 << 16) | (color2 << 12 | color2 << 8) | (color3 << 4 | color3));

        }
    case PIXEL_FORMAT_RGB565:
        switch (outputPixelFormat) {
        case PIXEL_FORMAT_ARGB8888: //RGB565 to RGB8888
            color1 = (color & 0x1f << 11) >> 11;
            color2 = (color & 0x3f << 5) >> 5;
            color3 = (color & 0x1f);
            color1 = (color1 * 527 + 23) >> 6;
            color2 = (color2 * 259 + 33) >> 6;
            color3 = (color3 * 527 + 23) >> 6;
            return (uint32_t)((color1 << 16) | (color2 << 8) | (color3) | 0xff000000);
        }
    case PIXEL_FORMAT_A8:
        switch (outputPixelFormat) {
        case PIXEL_FORMAT_ARGB8888:
            return (color & 0xFF) << 24;
        }
    }
    return 0;
}

void DisplayAntialiasedStringAt(const GUIFont* Font, int16_t Xpos, int16_t Ypos, const char* Text, uint32_t text_color) {

    while (*Text != 0) {
        uint32_t charCode = 0, nextCharCode = 0;
        int length = 1, charWidth = 0;
        GUIFont::unicode_character uchar, next_uchar;

        /* Display one character on LCD */
        if(*Text != 0x20) { //Not a space
            if((unsigned char)(*Text) > 0x7F) { //Unicode
                uchar = GUIFont::getUnicodeCharacter(Text);
                length = uchar.length;
                charCode = uchar.code;
                if((*(Text + length)) != 0) {
                    next_uchar = GUIFont::getUnicodeCharacter(Text + length);
                    nextCharCode = next_uchar.code;
                }
            } else { //UTF8
                charCode = *Text;
                if((*(Text + length)) != 0) {
                    if((*(Text + length)) <= 0x7f) {
                        nextCharCode = *(Text + length);
                    } else { //Next character is a unicode character
                        next_uchar = GUIFont::getUnicodeCharacter(Text + length);
                        nextCharCode = next_uchar.code;
                    }
                }
            }

            DisplayAntialiasedChar(Font, Xpos, Ypos, charCode, text_color);
            Xpos += Font->GetCharWidth(charCode);
        } else { //space
            charCode = *Text;
            nextCharCode = *(Text + length);
            charWidth = Font->GetBasicLength() * 2;
        }

        Xpos += charWidth;
        if(nextCharCode) Xpos += Font->GetKerning(charCode, nextCharCode);
        Text += length;
    }
}

void DisplayAntialiasedChar(const GUIFont* Font, uint16_t Xpos, uint16_t Ypos, uint32_t index, uint32_t text_color) {
    DrawAntialiasedChar(Font, Xpos, Ypos, Font->GetCharData(index), index, text_color);
}

void DrawAntialiasedChar(const GUIFont* Font, int16_t Xpos, int16_t Ypos, const uint8_t* c, uint32_t Index, uint32_t text_color) {
    DrawAntialiasedCharInBaund(Font, Xpos, Ypos, 0, 0, GetXSize(), GetYSize(), c, Index, text_color);
}

void DrawAntialiasedCharInBaund(const GUIFont* Font, int16_t Xpos, int16_t Ypos, int16_t ParentX, int16_t ParentY, int16_t ParentWidth, int16_t ParentHeight, const uint8_t* c, uint32_t Index, uint32_t text_color) {
    if(c == 0) return; //Character doesn't exists
    uint16_t height, width;
    int32_t TopOffset = 0, BottomOffset = 0;

    if(ParentHeight == 0) {
        return;
    }

    height = Font->GetHeight();
    width = Font->GetCharWidth(Index);

    int myY = Ypos - ParentY;

    if(ParentHeight < 0) {
        TopOffset = -ParentHeight;
        TopOffset = myY - TopOffset;
        if(TopOffset > 0) TopOffset = 0;
        else TopOffset = -TopOffset;
    } else if(ParentHeight <= myY + height) { //Bottom offset
        BottomOffset = height - (ParentHeight - myY);
    }

    if(TopOffset + BottomOffset >= height) return;

    DMA_MoveFont((uintptr_t)c, GetActiveLayerPointer(), 0, TopOffset, Xpos, Ypos + TopOffset, (int32_t)width,
    (int32_t)height - (BottomOffset + TopOffset), (int32_t)width, (int32_t)height, GetLayerPixelFormat(), text_color);
}

static void DMA_MoveFont(uintptr_t font_src, uintptr_t font_dst, int32_t x_src, int32_t y_src, int32_t x_dst, int32_t y_dst, int32_t width, int32_t height, int32_t fontWidth, int32_t fontHeight, uint32_t outPixelFormat, uint32_t fontColor) {
    float inBytes = 8 / 8;
    int outBytes = 2;
    uint32_t x_lcd_size = GetXSize();
    uintptr_t inputMem = 0, outputMem = 0;
    uint32_t NumberOfLines = 0, PixelsPerLine = 0, inOffset = 0, outOffset = 0;

    inputMem = font_src + (uint32_t)(inBytes * ((fontWidth * y_src) + x_src));
    outputMem = font_dst + (uint32_t)(outBytes * ((x_lcd_size * (y_dst)) + x_dst));
    NumberOfLines = height;
    PixelsPerLine = width;
    inOffset = fontWidth - width + x_src;
    outOffset = x_lcd_size - width;

    DMA_Operation(inputMem, outputMem, outputMem, PixelsPerLine, NumberOfLines, inOffset, outOffset, outOffset, 9,
    outPixelFormat, outPixelFormat, fontColor);
}
