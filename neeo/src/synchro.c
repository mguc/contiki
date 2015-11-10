/*
 * synchro.c
 *
 *  Created on: 22 Dec 2014
 *      Author: ws
 */

#include "synchro.h"
#include <stdint.h>

cyg_flag_t flag;

void SynchroInit() {
    cyg_flag_init(&flag);
}

void WaitForReload() {
    *(uint32_t*)0x40016840 = LAST_LINE;
    cyg_flag_maskbits(&flag, 0x3);
    cyg_flag_wait(&flag, 0x1, CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
}

void WaitForFirstLine() {
    *(uint32_t*)0x40016840 = FIRST_LINE;
    cyg_flag_maskbits(&flag, 0x3);
    cyg_flag_wait(&flag, 0x2, CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR);
}

inline void ReloadCompleted() {
    cyg_flag_setbits(&flag, 0x1);
}

inline void FirstLineAchieved() {
    cyg_flag_setbits(&flag, 0x2);
}
