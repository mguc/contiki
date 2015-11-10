#include "dma2d.h"
#include "log_helper.h"
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
#define PIXEL_FORMAT_RGB565  2
#define PIXEL_FORMAT_RGB888 1
#define PIXEL_FORMAT_ARGB8888 0

static uint32_t cconvert(uint32_t pdata, uint32_t format) {
   uint32_t c1 = pdata & 0xFF000000;
   uint32_t c2 = pdata & 0x00FF0000;
   uint32_t c3 = pdata & 0x0000FF00;
   uint32_t c4 = pdata & 0x000000FF;
   switch (format) {
  	case PIXEL_FORMAT_ARGB8888:
      		return (c3 | c2 | c1| c4);
        case PIXEL_FORMAT_RGB888:
                return (c3 | c2 | c4);
    	case PIXEL_FORMAT_RGB565:
   	    	c2 = (c2 >> 19);
	        c3 = (c3 >> 10);
	        c4 = (c4 >> 3 );
	        return ((c3 << 5) | (c2 << 11) | c4);
        case PIXEL_FORMAT_ARGB1555:
	      	c1 = (c1 >> 31);
	        c2 = (c2 >> 19);
	        c3 = (c3 >> 11);
	        c4 = (c4 >> 3 );
	        return ((c3 << 5) | (c2 << 10) | (c1 << 15) | c4);
        case PIXEL_FORMAT_ARGB4444:
      		c1 = (c1 >> 28);
      		c2 = (c2 >> 20);
      		c3 = (c3 >> 12);
      		c4 = (c4 >> 4 );
      		return ((c3 << 4) | (c2 << 8) | (c1 << 12) | c4);
         default:
		// TODO: unsupported
		return 0;
    }

}

void dma2d_dma_fill(uintptr_t outputMem, uint32_t PixelsPerLine, uint32_t NumberOfLines, uint32_t outOffset, uint32_t color_index, uint32_t outPixelFormat) {
  if ((PixelsPerLine == 0) || (NumberOfLines == 0) || (PixelsPerLine > 1024) || (NumberOfLines > 1024) || (outOffset > 0x3FFF)) {
  	log_msg(LOG_ERROR, "dma", "ERROR: Width/Height is wrong");
	return;
  }
  uint32_t val;
  val = R32(_DMA2D_OFFSET + _DMA2D_OPFCCR) & (uint32_t)~_DMA2D_OPFCCR_CM;
  val |= outPixelFormat;
  W32(_DMA2D_OFFSET + _DMA2D_OPFCCR, val);
  val = R32(_DMA2D_OFFSET + _DMA2D_OOR) & (uint32_t)~_DMA2D_OOR_LO;
  val |= outOffset;
  W32(_DMA2D_OFFSET + _DMA2D_OOR, val);
  W32(_DMA2D_OFFSET + _DMA2D_NLR, NumberOfLines | (PixelsPerLine << 16));
  W32(_DMA2D_OFFSET + _DMA2D_OMAR, outputMem);
  W32(_DMA2D_OFFSET + _DMA2D_OCOLR, cconvert(color_index, outPixelFormat));
  val = R32(_DMA2D_OFFSET + _DMA2D_CR) & (uint32_t)~(_DMA2D_CR_START | _DMA2D_CR_MODE_MASK);
  val |= _DMA2D_CR_MODE_R2M;
  val |= _DMA2D_CR_TCIE;
  val |= _DMA2D_CR_START;
  W32(_DMA2D_OFFSET + _DMA2D_CR, val);
}

void dma2d_dma_operation(uintptr_t inputMem, uintptr_t backgroundMem, uintptr_t outputMem, uint32_t PixelsPerLine,
        uint32_t NumberOfLines, uint32_t inOffset, uint32_t backgroundOffset, uint32_t outOffset, uint32_t inPixelFormat, uint32_t backgroundPixelFormat, uint32_t outPixelFormat, uint32_t frontColor) {
    if ((PixelsPerLine == 0) || (NumberOfLines == 0) || (PixelsPerLine > 1024) || (NumberOfLines > 1024) || (outOffset > 0x3FFF)) {
        log_msg(LOG_ERROR, "dma", "ERROR: Width/Height is wrong");
        return;
    }

    uint32_t val;
    val = R32(_DMA2D_OFFSET + _DMA2D_OPFCCR) & (uint32_t)~_DMA2D_OPFCCR_CM;
    val |= outPixelFormat;
    W32(_DMA2D_OFFSET + _DMA2D_OPFCCR, val);
    val = R32(_DMA2D_OFFSET + _DMA2D_OOR) & (uint32_t)~_DMA2D_OOR_LO;
    val |= outOffset;
    W32(_DMA2D_OFFSET + _DMA2D_OOR, val);

    if (backgroundMem != 0) {
        // background layer
        uint32_t val = R32(_DMA2D_OFFSET + _DMA2D_BGPFCCR) & (uint32_t)~(_DMA2D_BGPFCCR_CM | _DMA2D_BGPFCCR_AM | _DMA2D_BGPFCCR_ALPHA);
	val |= backgroundPixelFormat | (_DMA2D_NO_MODIF_ALPHA << 16);
	if ((backgroundPixelFormat != PIXEL_FORMAT_A4) && (backgroundPixelFormat != PIXEL_FORMAT_A8)) val |= 0xFF000000;
	W32(_DMA2D_OFFSET + _DMA2D_BGPFCCR, val);
	val = R32(_DMA2D_OFFSET + _DMA2D_BGOR) & (uint32_t)~_DMA2D_BGOR_LO;
	val |= backgroundOffset;
	W32(_DMA2D_OFFSET + _DMA2D_BGOR, val);
	if ((backgroundPixelFormat == PIXEL_FORMAT_A4) || (backgroundPixelFormat == PIXEL_FORMAT_A8)) W32(_DMA2D_OFFSET + _DMA2D_BGCOLR, 0xFF);
    }

    // foreground layer
    val = R32(_DMA2D_OFFSET + _DMA2D_FGPFCCR) & (uint32_t)~(_DMA2D_FGPFCCR_CM | _DMA2D_FGPFCCR_AM | _DMA2D_FGPFCCR_ALPHA);
    val |= inPixelFormat | (_DMA2D_NO_MODIF_ALPHA << 16);
    if ((inPixelFormat == PIXEL_FORMAT_A4) || (inPixelFormat == PIXEL_FORMAT_A8)) {
      val |= (frontColor & 0xFF000000);
    } else {
      val |= 0xFF000000;
    }
    W32(_DMA2D_OFFSET + _DMA2D_FGPFCCR, val);
    val = R32(_DMA2D_OFFSET + _DMA2D_FGOR) & (uint32_t)~_DMA2D_FGOR_LO;
    val |= inOffset;
    W32(_DMA2D_OFFSET + _DMA2D_FGOR, val);
    if ((inPixelFormat == PIXEL_FORMAT_A4) || (inPixelFormat == PIXEL_FORMAT_A8)) {
      W32(_DMA2D_OFFSET + _DMA2D_FGCOLR, frontColor & 0x00FFFFFF);
    }

    // do the operation
    W32(_DMA2D_OFFSET + _DMA2D_NLR, NumberOfLines | (PixelsPerLine << 16));
    W32(_DMA2D_OFFSET + _DMA2D_OMAR, (uint32_t)outputMem);
    W32(_DMA2D_OFFSET + _DMA2D_FGMAR, (uint32_t)inputMem);
    W32(_DMA2D_OFFSET + _DMA2D_BGMAR, (uint32_t)backgroundMem);
    val = R32(_DMA2D_OFFSET + _DMA2D_CR) & (uint32_t)~(_DMA2D_CR_START | _DMA2D_CR_MODE_MASK);
    if (backgroundMem != 0) {
            val |= _DMA2D_CR_MODE_M2M_BLEND;
    } else if (inPixelFormat == outPixelFormat) {
            val |= _DMA2D_CR_MODE_M2M;
    } else {
            val |= _DMA2D_CR_MODE_M2M_PFC;
    }
    val |= _DMA2D_CR_TCIE;
    val |= _DMA2D_CR_START;
    W32(_DMA2D_OFFSET + _DMA2D_CR, val);
}
