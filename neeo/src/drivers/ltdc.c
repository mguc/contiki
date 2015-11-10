#include "ltdc.h"
#include "board_config.h"
#include <stdint.h>

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

static int get_format_size(uint32_t format) {
	switch (format) {
		case PIXEL_FORMAT_ARGB8888:
			return 4;
		case PIXEL_FORMAT_RGB888:
			return 3;
		case PIXEL_FORMAT_ARGB4444:
		case PIXEL_FORMAT_RGB565:
		case PIXEL_FORMAT_ARGB1555:
		case PIXEL_FORMAT_AL88:
			return 2;
		default:
			return 1;
	}
}

void LTDC_SetConfig(int id, uint16_t Width, uint16_t Height, int DisplayPixelFormat, uint32_t addr) {
        uint32_t val, val2;
	val = (Width + ((R32(_LTDC_OFFSET + _LTDC_BPCR) & _LTDC_BPCR_AHBP) >> 16)) << 16;
	val |= (0 + ((R32(_LTDC_OFFSET + _LTDC_BPCR) & _LTDC_BPCR_AHBP) >> 16) + 1);
        val2 = R32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxWHPCR) & ~(_LTDC_LxWHPCR_WHSTPOS | _LTDC_LxWHPCR_WHSPPOS);
        W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxWHPCR, val2 | val);
	val = (Height + (R32(_LTDC_OFFSET + _LTDC_BPCR) & _LTDC_BPCR_AVBP)) << 16;
        val |= (0 + (R32(_LTDC_OFFSET + _LTDC_BPCR) & _LTDC_BPCR_AVBP) + 1);
	val2 = R32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxWVPCR) & ~(_LTDC_LxWVPCR_WVSTPOS | _LTDC_LxWVPCR_WVSPPOS);
	W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxWVPCR, val2 | val);
	W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxPFCR, DisplayPixelFormat);
	W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxDCCR, 0x00FFFFFF);
	W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxCACR, 0xFF);
	W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxBFCR, _LTDC_BLENDING_FACTOR1_PAxCA | _LTDC_BLENDING_FACTOR2_PAxCA);
        W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxCFBAR, addr);
	W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxCFBLR, (((Width * get_format_size(DisplayPixelFormat)) << 16) | ((Width * get_format_size(DisplayPixelFormat)) + 3)));
	W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxCFBLNR, Height);
	val = R32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxCR);
	W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxCR, val | _LTDC_LxCR_LEN);
	W32(_LTDC_OFFSET + _LTDC_SRCR, _LTDC_SRCR_IMR);
}


void LTDC_Init() {
        // TODO: why everything has -1 ?
	// TODO: we should support a struct like in linux
	uint32_t val = R32(_LTDC_OFFSET + _LTDC_GCR) & ~(_LTDC_GCR_HSPOL | _LTDC_GCR_VSPOL | _LTDC_GCR_DEPOL | _LTDC_GCR_PCPOL);
	val |= _LTDC_HSPOLARITY_AL | _LTDC_VSPOLARITY_AL | _LTDC_DEPOLARITY_AL | LCD_PCPolarity;
	W32(_LTDC_OFFSET + _LTDC_GCR, val);
	val = R32(_LTDC_OFFSET + _LTDC_SSCR) & ~(_LTDC_SSCR_VSH | _LTDC_SSCR_HSW);
	val |= ((LCD_HSYNC - 1) << 16) | (LCD_VSYNC -1);
	W32(_LTDC_OFFSET + _LTDC_SSCR, val);
	val = R32(_LTDC_OFFSET + _LTDC_BPCR) & ~(_LTDC_BPCR_AVBP | _LTDC_BPCR_AHBP);
	val |= ((LCD_HSYNC + LCD_HBP - 1) << 16) | (LCD_VSYNC + LCD_VBP - 1);
	W32(_LTDC_OFFSET + _LTDC_BPCR, val);
	val = R32(_LTDC_OFFSET + _LTDC_AWCR) & ~(_LTDC_AWCR_AAH | _LTDC_AWCR_AAW);
	val |= ((LCD_WIDTH + LCD_HSYNC + LCD_HBP - 1) << 16) | (LCD_HEIGHT + LCD_VSYNC + LCD_VBP - 1);
	W32(_LTDC_OFFSET + _LTDC_AWCR, val);
	val = R32(_LTDC_OFFSET + _LTDC_TWCR) & ~(_LTDC_TWCR_TOTALH | _LTDC_TWCR_TOTALW);
	val |= ((LCD_WIDTH + LCD_HSYNC + LCD_HBP + LCD_HFP - 1) << 16) | (LCD_HEIGHT + LCD_VSYNC + LCD_VBP + LCD_VFP - 1);
	W32(_LTDC_OFFSET + _LTDC_TWCR, val);
	val = R32(_LTDC_OFFSET + _LTDC_BCCR) & ~(_LTDC_BCCR_BCBLUE | _LTDC_BCCR_BCGREEN | _LTDC_BCCR_BCRED);
	val |= 0xFFFFFF00;
	W32(_LTDC_OFFSET + _LTDC_BCCR, val);
	val = R32(_LTDC_OFFSET + _LTDC_GCR);
	W32(_LTDC_OFFSET + _LTDC_GCR, val | _LTDC_GCR_LTDCEN);
}

// TODO: add lock here
void SetLayerPointer(uint32_t id, uintptr_t ptr) {
	W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxBFBAR, (uint32_t)ptr);
        W32(_LTDC_OFFSET + _LTDC_SRCR, _LTDC_SRCR_IMR);
}

void SetLayerEnable(uint32_t id, bool enable) {
    uint32_t val;
    val = R32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxCR);
    if (enable) {
       val |= _LTDC_LxCR_LAYER_ENABLE;
    } else {
       val &= ~_LTDC_LxCR_LAYER_ENABLE;
    }
    W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxCR, val);
    W32(_LTDC_OFFSET + _LTDC_SRCR, _LTDC_SRCR_IMR);
}

void SetLTDCEnable(bool enable) {
    uint32_t val;
    val = R32(_LTDC_OFFSET + _LTDC_GCR);
    if (enable) {
       val |= _LTDC_ENABLE;
    } else {
       val &= ~_LTDC_ENABLE;
    }
    W32(_LTDC_OFFSET + _LTDC_GCR, val);
}

void SetBackgroundColour(uint32_t col) {
        uint32_t val = R32(_LTDC_OFFSET + _LTDC_BCCR);
        val = (val & 0xFF000000) | (col & 0x00FFFFFFFF);
        W32(_LTDC_OFFSET + _LTDC_BCCR, val);
}

void SetLayerTransparency(uint32_t id, uint8_t value) {
        W32(_LTDC_OFFSET + _LTDC_LAYER_OFFSET(id) + _LTDC_LxCACR, value);
        W32(_LTDC_OFFSET + _LTDC_SRCR, _LTDC_SRCR_IMR);
}

#if 0
void vsync() {
  while (!(R32(_LTDC_OFFSET + _LTDC_CDSR) & LTDC_CDSR_VSYNCS)) {
        cyg_thread_yield();
  }
}
#endif


