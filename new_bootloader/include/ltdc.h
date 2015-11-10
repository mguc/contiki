#ifndef __LTDC_H__
#define __LTDC_H__

#include <stdint.h>

#define _LTDC_OFFSET 0x40016800
#define _LTDC_LIPCR 0x40 // line interrupt position
#define _LTDC_ICR 0x3C // interrupt clear
#define _LTDC_IER 0x34
#define _LTDC_GCR 0x18
#define _LTDC_GCR_LTDCEN 0x00000001
#define _LTDC_SSCR 0x8
#define _LTDC_BPCR 0xC
#define _LTDC_AWCR 0x10
#define _LTDC_TWCR 0x14
#define _LTDC_GCR_HSPOL 0x80000000
#define _LTDC_GCR_VSPOL 0x40000000
#define _LTDC_GCR_DEPOL 0x20000000
#define _LTDC_GCR_PCPOL 0x10000000
#define _LTDC_SSCR_VSH  0x000007FF
#define _LTDC_SSCR_HSW  0x0FFF0000
#define _LTDC_BPCR_AVBP 0x000007FF
#define _LTDC_BPCR_AHBP 0x0FFF0000
#define _LTDC_AWCR_AAH  0x000007FF
#define _LTDC_AWCR_AAW  0x0FFF0000
#define _LTDC_BCCR_BCGREEN 0x0000FF00
#define _LTDC_TWCR_TOTALW  0x0FFF0000
#define _LTDC_TWCR_TOTALH 0x000007FF
#define _LTDC_BCCR_BCBLUE 0x000000FF
#define _LTDC_BCCR_BCRED 0x00FF0000
#define _LTDC_ENABLE 0x1
#define _LTDC_IT_LI 0x1
#define _LTDC_ICR_CLIF 0x1
#define _LTDC_ICR_CRRIF 0x8
#define _LTDC_SRCR 0x24
#define _LTDC_SRCR_IMR 0x1
#define _LTDC_BCCR 0x2c
#define _LTDC_LAYER_OFFSET(n) (0x80 * n)
#define _LTDC_LxBFBAR 0xAC
#define _LTDC_LxCR 0x84
#define _LTDC_LxCR_LAYER_ENABLE 0x1
#define _LTDC_LxCACR 0x98
#define _LTDC_CDSR 0x48
#define _LTDC_CDSR_VSYNCS 0x4 // vert sync status
#define _LTDC_HSPOLARITY_AL 0x00000000
#define _LTDC_HSPOLARITY_AH 0x80000000
#define _LTDC_DEPOLARITY_AL 0x00000000
#define _LTDC_DEPOLARITY_AH 0x20000000
#define _LTDC_VSPOLARITY_AL 0x00000000
#define _LTDC_PCPOLARITY_IPC 0x00000000

#define _LTDC_LxWHPCR 0x88
#define _LTDC_LxWHPCR_WHSTPOS 0x00000FFF
#define _LTDC_LxWHPCR_WHSPPOS 0xFFFF0000
#define _LTDC_LxWVPCR 0x8C    
#define _LTDC_LxWVPCR_WVSTPOS 0x00000FFF
#define _LTDC_LxWVPCR_WVSPPOS 0xFFFF0000
#define _LTDC_LxPFCR 0x94
#define _LTDC_LxDCCR 0x9C
#define _LTDC_LxBFCR 0xA0
#define _LTDC_BLENDING_FACTOR1_PAxCA 0x00000600
#define _LTDC_BLENDING_FACTOR2_PAxCA 0x00000007
#define _LTDC_LxCFBAR 0xAC
#define _LTDC_LxCFBLR 0xB0
#define _LTDC_LxCFBLNR 0xB4
#define _LTDC_LxCR_LEN 0x00000001


#ifdef __cplusplus
extern "C"
#endif
void SetLayerPointer(uint32_t id, uintptr_t ptr);

#ifdef __cplusplus
extern "C"
#endif
void SetLayerEnable(uint32_t id, bool enable);

#ifdef __cplusplus
extern "C"
#endif
void SetLTDCEnable(bool enable);

#ifdef __cplusplus
extern "C"
#endif
void SetBackgroundColour(uint32_t col);

#ifdef __cplusplus
extern "C"
#endif
void SetLayerTransparency(uint32_t id, uint8_t value);

#ifdef __cplusplus
extern "C"
#endif
void LTDC_Init();

#ifdef __cplusplus
extern "C"
#endif
void LTDC_SetConfig(int id, uint16_t Width, uint16_t Height, int DisplayPixelFormat, uint32_t addr);


#endif
