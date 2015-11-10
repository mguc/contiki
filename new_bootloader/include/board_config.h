#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

///Display
#define LCD_F_WVGA_3262DG

//Board
#define BOARD_TR2_V2

///Color format - depth in bytes. can be 2 or 4
#define FRONT_DEPTH 2
#define BACK_DEPTH 2

/*
 * Thread Priorites
 * Prio 1: Hightest, Prio 31: Lowest
 */
#define DISPLAY_THREAD_PRIO 13
#define KEYBOARD_THREAD_PRIO 13
#define TOUCH_THREAD_PRIO 5
#define STATEDETECTION_THREAD_PRIO 20
#define BATTERY_THREAD_PRIO 20
#define JN_THREAD_PRIO 20
#define FIRMWAREUPDATE_THREAD_PRIO 20
#define GUIINIT_THREAD_PRIO 20
#define MAIN_THREAD_PRIO 30
#define FPS_THREAD_PRIO 31

#ifdef BOARD_TR2_V2
#define BOARD_TR2
#endif

#ifdef BOARD_TR2_V2
#define BOARD_NAME "TR2_V2"
#endif

// Display 480x800
#ifdef LCD_F_WVGA_3262DG
/**
  * @brief  LCD SPI
  */

/** 
  * @brief  LCD Size  
  */
#define  LCD_WIDTH            ((uint16_t)480)   /* LCD PIXEL WIDTH            */
#define  LCD_HEIGHT   	      ((uint16_t)800)   /* LCD PIXEL HEIGHT           */

/** 
  * @brief  LCD Timing  
  */
#define  LCD_HSYNC            ((uint16_t)2)     /* Horizontal synchronization */
#define  LCD_HBP              ((uint16_t)10)    /* Horizontal back porch      */
#define  LCD_HFP              ((uint16_t)10)    /* Horizontal front porch     */
#define  LCD_VSYNC            ((uint16_t)2)     /* Vertical synchronization   */
#define  LCD_VBP              ((uint16_t)10)    /* Vertical back porch        */
#define  LCD_VFP              ((uint16_t)10)    /* Vertical front porch       */
#define  LCD_PIXCLK           250 // 25 MHz

#define  LCD_PCPolarity       _LTDC_PCPOLARITY_IPC

#endif /* LCD_F_WVGA_3262DG */

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_CONFIG_H */
