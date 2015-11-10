#include <stdint.h>
#include "flashfs.h"
#include <stdio.h>
#include <sys/stat.h>
#include <cyg/fileio/fileio.h>
#include <cyg/io/spi.h>
#include <cyg/io/spi_stm32.h>
#include <cyg/hal/var_io.h>
#include <cyg/io/m25pxx.h>
#include "board_config.h"

#ifdef BOARD_EVAL
#define FLASH_RST CYGHWR_HAL_STM32_PIN_OUT(C, 4, PUSHPULL, NONE, FAST)
#endif

// static int is_mounted = 0;
// static FILE* open_file(char* path);

// extern void* custom_malloc(int size);
// extern void custom_free(void* p);

cyg_spi_cortexm_stm32_device_t flash_spi = {
#ifdef BOARD_TR2
    .spi_device.spi_bus = &cyg_spi_stm32_bus5.spi_bus,
#endif
#ifdef BOARD_EVAL
    .spi_device.spi_bus = &cyg_spi_stm32_bus2.spi_bus,
#endif
    .dev_num = 0,
    .cl_pol = 1,
    .cl_pha = 1,
    .cl_brate = 20000000,
    .cs_up_udly = 0,
    .cs_dw_udly = 0,
    .tr_bt_udly = 0,
    .bus_16bit = false,
};

CYG_DEVS_FLASH_SPI_M25PXX_DRIVER(m25pxx_flash_device, 0, &flash_spi.spi_device);

uint32_t flashfs_probe(void)
{
    uint8_t cmd[4] = {0, 0, 0, 0};
    uint8_t rx_buf[4];
    uint32_t id = 0;

    /* check WREN */
    cmd[0] = 0x05;
    cmd[1] = 0;
    cyg_spi_transfer (&flash_spi.spi_device, true, 2, cmd, rx_buf);

    cmd[0] = 0x9F;
    cmd[1] = 0;
    cmd[2] = 0;
    cmd[3] = 0;

    // Carry out SPI transfer.
    cyg_spi_transfer (&flash_spi.spi_device, true, 4, cmd, rx_buf);
    
    // Convert 3-byte ID to 32-bit integer.
    id |= ((uint32_t) rx_buf[1]) << 16;
    id |= ((uint32_t) rx_buf[2]) << 8;
    id |= ((uint32_t) rx_buf[3]); 

    return id;
}  

int flashfs_mount()
{
    int ret;

#ifdef BOARD_EVAL
    hal_stm32_gpio_set(FLASH_RST);
    hal_stm32_gpio_out(FLASH_RST, 1);
    hal_stm32_gpio_out(FLASH_RST, 0);
    hal_stm32_gpio_out(FLASH_RST, 1);
#endif
    ret = cyg_flash_init(NULL);
    
    ret = mount("/dev/flash/0/0,0x1000000", "/flash", "jffs2");
 
    if(ret) {
        flashfs_erase();
        
        ret = mount("/dev/flash/0/0,0x1000000", "/flash", "jffs2");
    }

    return ret;
}


void flashfs_erase(void) {
cyg_uint8 cmd[4] = {0, 0, 0, 0};
cyg_uint8 rx_buf[256], i;

    /* set WREN */
    cmd[0] = 0x06;
    cyg_spi_transfer(&flash_spi.spi_device, true, 1, cmd, NULL);

    /* check WREN */
    cmd[0] = 0x05;
    cmd[1] = 0;
    cyg_spi_transfer (&flash_spi.spi_device, true, 2, cmd, rx_buf);
    printf("SR = 0x%02x\r\n", rx_buf[1]);

    rx_buf[0] = 0;
    rx_buf[1] = 0;
 
    /* sector erase */
    printf("Erasing...\r\n");
    cmd[0] = 0x60;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    cyg_spi_transfer(&flash_spi.spi_device, true, 1, cmd, NULL);
    for(i=0; i<60; i++)
    {
        printf("Erasing in progress %02u\r", i);
        cyg_thread_delay(1000);
    }
    printf("\r\n");

    /* check SR for erase complete */
    cmd[0] = 0x05;
    cmd[1] = 0;
    do {
        cyg_thread_delay(1000);
        cyg_spi_transfer (&flash_spi.spi_device, true, 2, cmd, rx_buf);
        printf("Erase status 0x%02X\r\n", rx_buf[1]);
    } while(rx_buf[1] & 0x01);
    printf("Erase complete!\r\n");
}


// void romfs_unmount()
// {
//     if (!is_mounted)
//     {
//         return;
//     }

//     is_mounted = 0;
//     umount("/rom");
// }

// long romfs_get_file_length(char* path)
// {
//     char buffer[128];
//     buffer[0] = 0;
//     strcat(buffer, "/rom/"); 
//     strncat(buffer, path, 120);

//     struct stat file_stat;
//     if (stat(buffer, &file_stat) == -1)
//     {
//         return -1;
//     }

//     return file_stat.st_size;
// }

// long romfs_get_file_content(char* path, uint8_t* buffer, long max_length)
// {
//     FILE *f = open_file(path);
//     if (f == NULL)
//     {
//         return -1;
//     }

//     uint8_t* position = buffer;
//     long counter = 0;
//     char ch;

//     while ((fread(&ch, 1, 1, f) == 1) && (counter < max_length)) 
//     {
//         *position = ch;
//         position++;
//         counter++;
//     }
//     fclose(f);

//     return counter;
// }

// uint8_t* romfs_allocate_and_get_file_content(char* path, bool add_slash_zero, long* allocated_length)
// {
//     long length = romfs_get_file_length(path);
//     if (length == -1)
//     {
//         log_msg(LOG_ERROR, __cfunc__, "%s file not found", path);
//         return NULL;
//     }
//     else
//     {
//         *allocated_length = (add_slash_zero ? length + 1 : length);
//         uint8_t* buffer = (uint8_t*) custom_malloc(*allocated_length);
//         if (romfs_get_file_content(path, buffer, length) != length)
//         {
//             log_msg(LOG_ERROR, __cfunc__, "error while reading data from file %s", path);
//             custom_free(buffer);
//             return NULL;
//         }
//         if (add_slash_zero)
//         {
//             buffer[length] = 0;
//         }

//         return buffer;
//     }
// }

// static FILE* open_file(char* path)
// {
//     char buffer[128];
//     buffer[0] = 0;
//     strcat(buffer, "/rom/"); 
//     strncat(buffer, path, 120);

//     return fopen(buffer, "rb");
// }


