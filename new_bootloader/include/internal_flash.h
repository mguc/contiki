/********************************************************************
 * 
 * CONFIDENTIAL
 * 
 * ---
 * 
 * (c) 2014-2015 Antmicro Ltd <antmicro.com>
 * All Rights Reserved.
 * 
 * NOTICE: All information contained herein is, and remains
 * the property of Antmicro Ltd.
 * The intellectual and technical concepts contained herein
 * are proprietary to Antmicro Ltd and are protected by trade secret
 * or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Antmicro Ltd.
 *
 * ---
 *
 * Created on: 26 Oct 2015
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef SRC_DRIVERS_INTERNAL_FLASH_H_
#define SRC_DRIVERS_INTERNAL_FLASH_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <cyg/fileio/fileio.h>
#include <cyg/hal/var_io.h>

#ifdef __cplusplus
extern "C" {
#endif

void (*BlockCallback)(int);

void InternalFlash_RegisterBlockCallback(void (*callback)(int));

int InternalFlash_Unlock(void);
void InternalFlash_Lock(void);

void InternalFlash_ProgramByte(uint32_t address, uint8_t* data);
int InternalFlash_WaitForLastOperation();
void InternalFlash_EraseSector(uint32_t sector);
void InternalFlash_ProgramBinary(uint32_t address, uint8_t* data, uint32_t length);
void InternalFlash_ProgramFromFile(uint32_t address, const char* path);

uint32_t InternalFlash_GetSectorSize(uint32_t sector);
uint32_t InternalFlash_GetSectorAddress(uint32_t sector);
uint32_t InternalFlash_GetSector(uint32_t address);

#ifdef __cplusplus
}
#endif

#endif /* NEW_BOOTLOADER_SRC_DRIVERS_INTERNAL_FLASH_H_ */
