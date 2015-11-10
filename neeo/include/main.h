#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#include "stl.h"
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/var_io.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <cyg/infra/diag.h>
#include <pkgconf/posix.h>
#include <cyg/io/io.h>
#include <cyg/io/serialio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "Manager.h"
#include "mem_alloc.h"
#include "stm324x9i_eval_lcd.h"
#include "stm324x9i_eval_sdram.h"
#include "stm32f4xx_hal.h"
#include "GUILib.h"
#include "synchro.h"

#ifdef BOARD_TR2
#include "tca9535.h"
#endif

#ifdef BOARD_TR2_V2
#include "tca6424.h"
#endif

#include "joy.h"
#include "file_download.h"
#include "png_helper.h"

//Drivers
#include "gt9147.h"
#include "pwm.h"
#include "FontManager.h"
#include "jn5168_flasher.h"
#include "cc3100.h"
#include "LIS2DH.h"

void Reset();

#endif /* __MAIN_H */
