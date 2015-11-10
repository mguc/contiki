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

#include "internal_flash.h"

#define SECTOR_MASK               ((uint32_t)0xFFFFFF07)
#define FLASH_KEY1               ((uint32_t)0x45670123)
#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)

#define FLASH_FLAG_EOP                 ((uint32_t)0x00000001)            /*!< FLASH End of Operation flag               */
#define FLASH_FLAG_OPERR               ((uint32_t)0x00000002)            /*!< FLASH operation Error flag                */
#define FLASH_FLAG_WRPERR              ((uint32_t)0x00000010)         /*!< FLASH Write protected error flag          */
#define FLASH_FLAG_PGAERR              ((uint32_t)0x00000020)         /*!< FLASH Programming Alignment error flag    */
#define FLASH_FLAG_PGPERR              ((uint32_t)0x00000040)         /*!< FLASH Programming Parallelism error flag  */
#define FLASH_FLAG_PGSERR              ((uint32_t)0x00000080)         /*!< FLASH Programming Sequence error flag     */
#define FLASH_FLAG_RDERR               ((uint32_t)0x00000100)  /*!< Read Protection error flag (PCROP)        */
#define FLASH_FLAG_BSY                 ((uint32_t)0x00010000)            /*!< FLASH Busy flag                           */

#define FLASH_PSIZE_BYTE           ((uint32_t)0x00000000)
#define FLASH_PSIZE_HALF_WORD      ((uint32_t)0x00000100)
#define FLASH_PSIZE_WORD           ((uint32_t)0x00000200)
#define FLASH_PSIZE_DOUBLE_WORD    ((uint32_t)0x00000300)
#define CR_PSIZE_MASK              ((uint32_t)0xFFFFFCFF)

void InternalFlash_RegisterBlockCallback(void (*callback)(int)){
    BlockCallback = callback;
}

int InternalFlash_Unlock(void) {
    if((FLASH->CR & FLASH_CR_LOCK) != 0) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    } else {
        return -1;
    }

    FLASH->SR = (FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    return 0;
}

void InternalFlash_Lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

void InternalFlash_ProgramByte(uint32_t address, uint8_t* data) {
    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= FLASH_PSIZE_BYTE;
    FLASH->CR |= FLASH_CR_PG;

    *(volatile uint8_t*)address = *data;
}

int InternalFlash_WaitForLastOperation() {
    while ((FLASH->SR & FLASH_FLAG_BSY) != 0) {
        //FixMe timeout removed
    }

    if((FLASH->SR & (FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR | FLASH_FLAG_RDERR)) != 0) {
        return -1;
    }

    return 0;
}

void InternalFlash_EraseSector(uint32_t sector) {
    /* Need to add offset of 4 when sector higher than FLASH_SECTOR_11 */
    if(sector > 11) {
        sector += 4;
    }
    /* If the previous operation is completed, proceed to erase the sector */
    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= 0;
    FLASH->CR &= SECTOR_MASK;
    FLASH->CR |= FLASH_CR_SER | (sector << POSITION_VAL(FLASH_CR_SNB));
    FLASH->CR |= FLASH_CR_STRT;
}

void InternalFlash_ProgramBinary(uint32_t address, uint8_t* data, uint32_t length) {
    uint32_t sector = InternalFlash_GetSector(address);
    if(sector == 0xffffffff) return;

    if(BlockCallback) { //Draw empty progress bar
        BlockCallback(0);
    }

    int remainingSpace = InternalFlash_GetSectorSize(sector) - (address - InternalFlash_GetSectorAddress(sector));

    int dataBatch = 0, dataLeft = 0;
    if(remainingSpace > length) {
        dataBatch = length;
        dataLeft = length;
    } else {
        dataBatch = remainingSpace;
        dataLeft = length;
    }

    do {
        printf("BOOT: Erasing sector %d \r\n", sector);
        InternalFlash_EraseSector(sector);
        InternalFlash_WaitForLastOperation();
        printf("BOOT: Programming sector %d \r\n", sector);
        int i=0;
        for (; i < dataBatch; i++) {
            InternalFlash_ProgramByte((uint32_t)(address), data);
            InternalFlash_WaitForLastOperation();
            ++address;
            ++data;
            --dataLeft;
        }

        if(BlockCallback){
            BlockCallback(((float)(length-dataLeft)/(float)length)*100);
        }

        ++sector;

        if(dataLeft > InternalFlash_GetSectorSize(sector)) {
            dataBatch = InternalFlash_GetSectorSize(sector);
        } else {
            dataBatch = dataLeft;
        }

    } while (dataLeft);

    if(BlockCallback){
        BlockCallback(100);
    }

}

void InternalFlash_ProgramFromFile(uint32_t address, const char* path){

    uint32_t sector = InternalFlash_GetSector(address);
    if(sector == 0xffffffff) return;

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        printf("Error - file cannot be opened\r\n");
        return;
    }

    struct stat file_stat;
    stat(path, &file_stat);
    uint32_t length = file_stat.st_size;

    int remainingSpace = InternalFlash_GetSectorSize(sector) - (address - InternalFlash_GetSectorAddress(sector));

    int dataBatch = 0, dataLeft = 0;
    if(remainingSpace > length) {
        dataBatch = length;
        dataLeft = length;
    } else {
        dataBatch = remainingSpace;
        dataLeft = length;
    }

     long counter = 0;
     uint8_t* data;

    do {
        printf("BOOT: Erasing sector %d \r\n", sector);
        InternalFlash_EraseSector(sector);
        InternalFlash_WaitForLastOperation();
        printf("BOOT: Programming sector %d \r\n", sector);
        int i=0;
        for (; i < dataBatch; i++) {
            fread(&data, 1, 1, f);
            ++counter;
            InternalFlash_ProgramByte((uint32_t)(address), data);
            InternalFlash_WaitForLastOperation();
            ++address;
            --dataLeft;
        }

        ++sector;

        if(dataLeft > InternalFlash_GetSectorSize(sector)) {
            dataBatch = InternalFlash_GetSectorSize(sector);
        } else {
            dataBatch = dataLeft;
        }

    } while (dataLeft);

    fclose(f);
}

uint32_t InternalFlash_GetSectorSize(uint32_t sector) {
    switch (sector) {
    case 0:
        return 16 * 1024;
    case 1:
        return 16 * 1024;
    case 2:
        return 16 * 1024;
    case 3:
        return 16 * 1024;
    case 4:
        return 64 * 1024;
    case 5:
        return 128 * 1024;
    case 6:
        return 128 * 1024;
    case 7:
        return 128 * 1024;
    case 8:
        return 128 * 1024;
    case 9:
        return 128 * 1024;
    case 10:
        return 128 * 1024;
    case 11:
        return 128 * 1024;
    case 12:
        return 16 * 1024;
    case 13:
        return 16 * 1024;
    case 14:
        return 16 * 1024;
    case 15:
        return 16 * 1024;
    case 16:
        return 64 * 1024;
    case 17:
        return 128 * 1024;
    case 18:
        return 128 * 1024;
    case 19:
        return 128 * 1024;
    case 20:
        return 128 * 1024;
    case 21:
        return 128 * 1024;
    case 22:
        return 128 * 1024;
    case 23:
        return 128 * 1024;
    default:
        return 0;
    }
}

uint32_t InternalFlash_GetSectorAddress(uint32_t sector) {
    switch (sector) {
    case 0:
        return 0x08000000;
    case 1:
        return 0x08004000;
    case 2:
        return 0x08008000;
    case 3:
        return 0x0800C000;
    case 4:
        return 0x08010000;
    case 5:
        return 0x08020000;
    case 6:
        return 0x08040000;
    case 7:
        return 0x08060000;
    case 8:
        return 0x08080000;
    case 9:
        return 0x080A0000;
    case 10:
        return 0x080C0000;
    case 11:
        return 0x080E0000;
    case 12:
        return 0x08100000;
    case 13:
        return 0x08104000;
    case 14:
        return 0x08108000;
    case 15:
        return 0x0810C000;
    case 16:
        return 0x08110000;
    case 17:
        return 0x08120000;
    case 18:
        return 0x08140000;
    case 19:
        return 0x08160000;
    case 20:
        return 0x08180000;
    case 21:
        return 0x081A0000;
    case 22:
        return 0x081C0000;
    case 23:
        return 0x081E0000;
    default:
        return 0xFFFFFFFF;
    }
}

uint32_t InternalFlash_GetSector(uint32_t address) {
    if(address >= 0x08000000 && address < 0x08004000) return 0;
    else if(address >= 0x08004000 && address < 0x08008000) return 1;
    else if(address >= 0x08008000 && address < 0x0800C000) return 2;
    else if(address >= 0x0800C000 && address < 0x08010000) return 3;
    else if(address >= 0x08010000 && address < 0x08020000) return 4;
    else if(address >= 0x08020000 && address < 0x08040000) return 5;
    else if(address >= 0x08040000 && address < 0x08060000) return 6;
    else if(address >= 0x08060000 && address < 0x08080000) return 7;
    else if(address >= 0x08080000 && address < 0x080A0000) return 8;
    else if(address >= 0x080A0000 && address < 0x080C0000) return 9;
    else if(address >= 0x080C0000 && address < 0x080E0000) return 10;
    else if(address >= 0x080E0000 && address < 0x08100000) return 11;
    else if(address >= 0x08100000 && address < 0x08104000) return 12;
    else if(address >= 0x08104000 && address < 0x08108000) return 13;
    else if(address >= 0x08108000 && address < 0x0810C000) return 14;
    else if(address >= 0x0810C000 && address < 0x08110000) return 15;
    else if(address >= 0x08110000 && address < 0x08120000) return 16;
    else if(address >= 0x08120000 && address < 0x08140000) return 17;
    else if(address >= 0x08140000 && address < 0x08160000) return 18;
    else if(address >= 0x08160000 && address < 0x08180000) return 19;
    else if(address >= 0x08180000 && address < 0x081A0000) return 20;
    else if(address >= 0x081A0000 && address < 0x081C0000) return 21;
    else if(address >= 0x081C0000 && address < 0x081E0000) return 22;
    else if(address >= 0x081E0000 && address < 0x08200000) return 23;
    else return 0xffffffff;
}
