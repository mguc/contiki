/*
 * spi.c - tiva-c launchpad spi interface implementation
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#ifndef SL_IF_TYPE_UART
#include <cyg/kernel/kapi.h>
#include <cyg/io/spi.h>
#include <cyg/io/spi_stm32.h>
#include <cyg/hal/var_io.h>
#include "simplelink.h"
#include "board.h"
#include "board_config.h"
#include "spi.h"

cyg_spi_cortexm_stm32_device_t spi_interface = {
#ifdef BOARD_EVAL
.spi_device.spi_bus = &cyg_spi_stm32_bus1.spi_bus,
#endif
#ifdef BOARD_TR2
.spi_device.spi_bus = &cyg_spi_stm32_bus3.spi_bus,
#endif
    .dev_num = 0,
    .cl_pol = 0,
    .cl_pha = 0,
    .cl_brate = 4000000,
    .cs_up_udly = 0,
    .cs_dw_udly = 0,
    .tr_bt_udly = 0,
    .bus_16bit = false,
};

#if 0
   #define debug_printf( __fmt, ... ) printf("(mmaj): %s[%d]: " __fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__ );
#else
   #define debug_printf(args...)
#endif

// software SPI, remove in production version
// #define nCS CYGHWR_HAL_STM32_PIN_OUT(B, 6, PUSHPULL, NONE, HIGH)
// #define SCK CYGHWR_HAL_STM32_PIN_OUT(A, 5, PUSHPULL, NONE, HIGH)
// #define MOSI CYGHWR_HAL_STM32_PIN_OUT(B, 5, PUSHPULL, NONE, HIGH)
// #define MISO CYGHWR_HAL_STM32_PIN_IN(A, 6, PULLUP)

int spi_Close(Fd_t fd)
{
    /* Disable WLAN Interrupt ... */
    CC3100_InterruptDisable();

    return NONOS_RET_OK;
}

Fd_t spi_Open(char *ifName, unsigned long flags)
{
	 debug_printf("spi config!\n\r");


    // initialize software SPI lines

//     hal_stm32_gpio_set(nCS);
//     hal_stm32_gpio_set(MOSI);
//     hal_stm32_gpio_set(MISO);
//     hal_stm32_gpio_set(SCK);
//     hal_stm32_gpio_out(nCS, 1);
//     hal_stm32_gpio_out(SCK, 0);
//     hal_stm32_gpio_out(MOSI, 0);


    /* Enable WLAN interrupt */
    CC3100_InterruptEnable();

    debug_printf("done!\n\r");
    
    return NONOS_RET_OK;
}

int spi_Write(Fd_t fd, unsigned char *pBuff, int len)
{
    // software SPI, remove in production version
//     int i, j;
//     cyg_uint8 temp;

    cyg_spi_transfer(&spi_interface.spi_device, true, len, pBuff, NULL);

     //software SPI, remove in production version
//     hal_stm32_gpio_out(nCS, 0);
//     for(i=0; i<len; i++)
//     {
//         temp = pBuff[i];
//         for(j=0; j<8; j++)
//         {
//             if(temp & 0x80)
//                 hal_stm32_gpio_out(MOSI, 1);
//             else
//                 hal_stm32_gpio_out(MOSI, 0);
//             hal_stm32_gpio_out(SCK, 1);
//             temp = temp << 1;
//             hal_stm32_gpio_out(SCK, 0);
//         }
//         hal_stm32_gpio_out(MOSI, 0);
//     }
//     hal_stm32_gpio_out(nCS, 1);


    return len;
}

int spi_Read(Fd_t fd, unsigned char *pBuff, int len)
{
    // software SPI, remove in production version
//     int i, j, bitval;

    cyg_spi_transfer(&spi_interface.spi_device, true, len, NULL, pBuff);

     //software SPI, remove in production version
//     hal_stm32_gpio_out(nCS, 0);
//     for(i=0; i<len; i++)
//     {
//         pBuff[i] = 0;
//         for(j=0; j<8; j++)
//         {
//             hal_stm32_gpio_out(SCK, 1);
//             hal_stm32_gpio_in(MISO, &bitval);
//             hal_stm32_gpio_out(SCK, 0);
//             pBuff[i] = pBuff[i] | (bitval << (7-j));
//         }
//     }
//     hal_stm32_gpio_out(nCS, 1);
    return len;
}
#endif /* SL_IF_TYPE_UART */
