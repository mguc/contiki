/*
 * synchro.h
 *
 *  Created on: 22 Dec 2014
 *      Author: ws
 */

#ifndef SYNCHRO_H_
#define SYNCHRO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/var_io.h>
#include <pthread.h>
#include <semaphore.h>
#include "board_config.h"

#define FIRST_LINE 0
#ifdef LCD_F_WVGA_3262DG
#define LAST_LINE 800
#else
#define LAST_LINE 480
#endif

void SynchroInit();
void WaitForReload();
void WaitForFirstLine();
void ReloadCompleted();
void FirstLineAchieved();

#ifdef __cplusplus
};
#endif


#endif /* SYNCHRO_H_ */
