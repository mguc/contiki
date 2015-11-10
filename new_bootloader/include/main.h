#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/var_io.h>
#include <cyg/io/io.h>
#include <cyg/io/serialio.h>
#include <cyg/crc/crc.h>
#include <cyg/io/flash.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>

#include "mem_alloc.h"
#include "stm324x9i_eval_lcd.h"
#include "stm324x9i_eval_sdram.h"
#include "stm32f4xx_hal.h"

//Drivers
#include "lg4573a.h"
#include "dma2d.h"
#include "ltdc.h"
#include "internal_flash.h"
#include "flashfs.h"
#include "tca6424.h"
#include "romfs.h"

//Libs
#include "draw.h"
#include "GUIFont.h"


#ifdef BOARD_EVAL
#define RESET_LEVEL 1
#define LED1 CYGHWR_HAL_STM32_PIN_OUT(G, 6, PUSHPULL, NONE, FAST)
#define LED2 CYGHWR_HAL_STM32_PIN_OUT(G, 7, PUSHPULL, NONE, FAST)
#define LED3 CYGHWR_HAL_STM32_PIN_OUT(G, 10, PUSHPULL, NONE, FAST)
#define LED4 CYGHWR_HAL_STM32_PIN_OUT(G, 12, PUSHPULL, NONE, FAST)
#define LCDBL_PWM CYGHWR_HAL_STM32_PIN_OUT(C, 6, PUSHPULL, NONE, FAST)
#define LCD_RESET CYGHWR_HAL_STM32_PIN_OUT(C, 7, PUSHPULL, PULLUP, FAST)
#define TOUCH_RESET CYGHWR_HAL_STM32_PIN_OUT(B, 11, PUSHPULL, NONE, FAST)
#define INTERRUPT_TOUCH CYGHWR_HAL_STM32_PIN_IN(F, 9, NONE)
#define INT_TOUCH_EXTI CYGNUM_HAL_INTERRUPT_EXTI9
#endif

#ifdef BOARD_TR2
#define RESET_LEVEL 0
#ifdef BOARD_TR2_V2
#define LCDBL_PWM CYGHWR_HAL_STM32_PIN_OUT(D, 13, PUSHPULL, PULLDOWN, FAST)
#define FPGA_EN CYGHWR_HAL_STM32_PIN_OUT(D, 12, PUSHPULL, PULLDOWN, FAST)
#else
#define LCDBL_PWM CYGHWR_HAL_STM32_PIN_OUT(A, 8, PUSHPULL, PULLDOWN, FAST)
#endif
#define LCD_RESET CYGHWR_HAL_STM32_PIN_OUT(C, 13, PUSHPULL, PULLDOWN, FAST)
#define TOUCH_RESET CYGHWR_HAL_STM32_PIN_OUT(A, 3, PUSHPULL, NONE, FAST)
#define INTERRUPT_TOUCH CYGHWR_HAL_STM32_PIN_IN(B, 1, NONE)
#define WIFI_SUP_EN CYGHWR_HAL_STM32_PIN_OUT(C, 7, PUSHPULL, NONE, HIGH)
#define CHG_MODE CYGHWR_HAL_STM32_PIN_OUT(D, 11, PUSHPULL, NONE, FAST)
#define CHG_CHRG CYGHWR_HAL_STM32_PIN_IN(B, 15, PULLUP)
#define INT_TOUCH_EXTI CYGNUM_HAL_INTERRUPT_EXTI1
#define LIS2DH_INT1 CYGHWR_HAL_STM32_PIN_IN(J, 8, NONE)
#define LIS2DH_INT2 CYGHWR_HAL_STM32_PIN_IN(J, 10, NONE)
#endif

#define KEY_OK      0x0100
#define KEY_RIGHT   0x0080
#define KEY_LEFT    0x0200
#define KEY_HOME    0x0002
#define KEY_MENU    0x1000
#define KEY_BACK    0x0010


#define W32(a, b) (*(volatile unsigned int   *)(a)) = b
#define R32(a)    (*(volatile unsigned int   *)(a))

#define LCD_COLOR_NEEOBLUE 0xFF000030

enum ResetSource {
    Unknown,
    PowerOn,
    Software,
    Watchdog,
    LowPower,
    WindowWatchdog,
    PORPDR,
    PIN,
    BOR,
};

#endif /* __MAIN_H */
