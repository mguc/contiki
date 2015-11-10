#ifndef __DMA2D_H__
#define __DMA2D_H__

#include <stdint.h>

#define _DMA2D_OFFSET 0x4002B000
#define _DMA2D_ISR 0x08
#define _DMA2D_CR 0x00
#define _DMA2D_CR_MODE_MASK 0x00030000
#define _DMA2D_CR_MODE_R2M 0x00030000
#define _DMA2D_CR_MODE_M2M 0x0
#define _DMA2D_CR_MODE_M2M_PFC 0x00010000
#define _DMA2D_CR_MODE_M2M_BLEND 0x00020000
#define _DMA2D_CR_START 0x1
#define _DMA2D_NLR 0x44
#define _DMA2D_OMAR 0x3C
#define _DMA2D_OCOLR 0x38
#define _DMA2D_FGPFCCR 0x1C
#define _DMA2D_FGOR 0x10
#define _DMA2D_BGMAR 0x14
#define _DMA2D_FGCOLR 0x20
#define _DMA2D_FGMAR 0x0C
#define _DMA2D_BGOR 0x18
#define _DMA2D_BGCOLR 0x28
#define _DMA2D_BGPFCCR 0x24
#define _DMA2D_OPFCCR 0x34
#define _DMA2D_OOR 0x40
#define _DMA2D_CR_TCIE 0x200
#define _DMA2D_ISR_TCIF 0x2
#define _DMA2D_NO_MODIF_ALPHA 0x0
#define _DMA2D_BGPFCCR_CM 0x0000000F
#define _DMA2D_BGPFCCR_AM 0x00030000
#define _DMA2D_BGPFCCR_ALPHA 0xFF000000
#define _DMA2D_FGPFCCR_CM 0x0000000F
#define _DMA2D_FGPFCCR_AM 0x00030000
#define _DMA2D_FGOR_LO 0x00003FFF
#define _DMA2D_BGOR_LO 0x00003FFF
#define _DMA2D_FGPFCCR_ALPHA 0xFF000000
#define _DMA2D_OPFCCR_CM 0x00000007
#define _DMA2D_OOR_LO 0x00003FFF

#ifdef __cplusplus
extern "C"
#endif
void dma2d_dma_fill(uintptr_t outputMem, uint32_t PixelsPerLine, uint32_t NumberOfLines, uint32_t outOffset, uint32_t color_index, uint32_t outPixelFormat);

#ifdef __cplusplus
extern "C"
#endif
void dma2d_dma_operation(uintptr_t inputMem, uintptr_t backgroundMem, uintptr_t outputMem, uint32_t PixelsPerLine,
        uint32_t NumberOfLines, uint32_t inOffset, uint32_t backgroundOffset, uint32_t outOffset, uint32_t inPixelFormat, uint32_t backgroundPixelFormat, uint32_t outPixelFormat, uint32_t frontColor);


#endif
