#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

//#define DEBUG_LCD

///Display
//#define LCD_ED0700G0DM6
//#define LCD_FUSION7
#define LCD_F_WVGA_3262DG

//#define ROTATE90

//Touchscreen
//#define GT_SCALE_DOWN

//Board
#define BOARD_EVAL
//#define BOARD_TR2
// #define BOARD_TR2_V2


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

#ifdef BOARD_EVAL
#define BOARD_NAME "EVAL + TR2 shield"
#endif

#ifdef BOARD_TR2_V2
#define BOARD_NAME "TR2_V2"
#else
#ifdef BOARD_TR2
#define BOARD_NAME "TR2"
#endif
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


#if 0
device tree method of describing panels. suggestion - base our struct on this:
                        timing4: timing4 {
                                panel-name = "ET0500", "ET0700";
                                clock-frequency = <33260000>;
                                hactive = <800>;
                                vactive = <480>;
                                hback-porch = <88>;
                                hsync-len = <128>;
                                hfront-porch = <40>;
                                vback-porch = <33>;
                                vsync-len = <2>;
                                vfront-porch = <10>;
                                hsync-active = <0>;
                                vsync-active = <0>;
                                de-active = <1>;
                                pixelclk-active = <1>;
                        };



2.We suggest you try the following values:

    Horizontal synchronization ：2 （PCLK）
    Horizontal back porch   ：10(PCLK)
    Horizontal front porch ：  10 (PCLK)
    Vertical synchronization ：  2
    Vertical back porch ：10
    Vertical front porch：10





#endif

// Display ED0700G0DM6 - @800x480 / LCD_FUSION7
#if defined(LCD_ED0700G0DM6) || defined(LCD_FUSION7)

/** 
  * @brief  LCD Size  
  */
#define  LCD_WIDTH            ((uint16_t)800)   /* LCD PIXEL WIDTH            */
#define  LCD_HEIGHT   	      ((uint16_t)480)   /* LCD PIXEL HEIGHT           */

/** 
  * @brief  LCD Timing  
  */
#define  LCD_HSYNC            ((uint16_t)182)   /* Horizontal synchronization */
#define  LCD_HBP              ((uint16_t)34)    /* Horizontal back porch      */
#define  LCD_HFP              ((uint16_t)40)    /* Horizontal front porch     */
#define  LCD_VSYNC            ((uint16_t)2)     /* Vertical synchronization   */
#define  LCD_VBP              ((uint16_t)35)    /* Vertical back porch        */
#define  LCD_VFP              ((uint16_t)8)     /* Vertical front porch       */
#define  LCD_PIXCLK           250 // 25 MHz


// TODO: WHY THIS BELOW? CANNOT IT BE IPC?
#ifdef LCD_ED0700G0DM6
 #define  LCD_PCPolarity       _LTDC_PCPOLARITY_IIPC
#endif

#ifdef LCD_FUSION7
#define  LCD_PCPolarity       _LTDC_PCPOLARITY_IPC
#endif /* LCD_FUSION7 */

#endif


#ifdef __cplusplus
}
#endif

#endif /* __BOARD_CONFIG_H */
