/*
 * board.c - tiva-c launchpad configuration
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

#include <cyg/kernel/kapi.h>
#include <cyg/hal/var_io.h>
#include <cyg/hal/var_io_pins.h>
#include <cyg/hal/var_intr.h>
#include "simplelink.h"
#include "board.h"
#include "board_config.h"

//EVAL
#ifdef BOARD_EVAL
#define NHIB CYGHWR_HAL_STM32_PIN_OUT(D, 2, PUSHPULL, NONE, HIGH)
#define INTERRUPT_INPUT CYGHWR_HAL_STM32_PIN_IN(G, 14, PULLDOWN)
#define WIFI_UART_TX CYGHWR_HAL_STM32_PIN_OUT(B, 10, PUSHPULL, NONE, HIGH)
#define WIFI_UART_RX CYGHWR_HAL_STM32_PIN_OUT(C, 11, PUSHPULL, NONE, HIGH)
#endif

#ifdef BOARD_TR2
#define SUP_EN CYGHWR_HAL_STM32_PIN_OUT(C, 7, PUSHPULL, NONE, HIGH)
#define INTERRUPT_INPUT CYGHWR_HAL_STM32_PIN_IN(D, 7, PULLDOWN)
#define NHIB CYGHWR_HAL_STM32_PIN_OUT(J, 12, PUSHPULL, NONE, HIGH)
#define NRST CYGHWR_HAL_STM32_PIN_OUT(J, 13, PUSHPULL, NONE, HIGH)

#endif

#if 0
   #define debug_printf( __fmt, ... ) printf("(mmaj): %s[%d]: " __fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__ );
#else
   #define debug_printf(args...)
#endif

P_EVENT_HANDLER        pIrqEventHandler = 0;

_u8 IntIsMasked;

cyg_handle_t isr_handle;
cyg_interrupt isr_object;
cyg_uint32 data;

void initClk(void)
{
    //FIXME this function should be called only once.
    static bool initialized = false;

    if(!initialized) {

    #ifdef BOARD_TR2
      hal_stm32_gpio_set(SUP_EN);
      hal_stm32_gpio_out(SUP_EN, 1);
      hal_stm32_gpio_set(NRST);
      hal_stm32_gpio_out(NRST, 1);
    #endif
    #ifdef BOARD_EVAL
      hal_stm32_gpio_set(WIFI_UART_TX);
      hal_stm32_gpio_set(WIFI_UART_RX);
      hal_stm32_gpio_out(WIFI_UART_TX, 1);
      hal_stm32_gpio_out(WIFI_UART_RX, 1);
    #endif

    hal_stm32_gpio_set(NHIB);
    hal_stm32_gpio_set(INTERRUPT_INPUT);

    cyg_uint32 regval;

#ifdef BOARD_EVAL
    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_EXTI14, 0, (cyg_addrword_t)&data, CC3100_intISR, CC3100_intDSR, &isr_handle, &isr_object);
    cyg_interrupt_configure(CYGNUM_HAL_INTERRUPT_EXTI14, 0, true);
    cyg_interrupt_attach(isr_handle);
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EXTI14);
    HAL_READ_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(14), regval);
    regval |= CYGHWR_HAL_STM32_SYSCFG_EXTICRX_PORTG(14);
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(14), regval);
#endif
#ifdef BOARD_TR2
    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_EXTI7, 0, (cyg_addrword_t)&data, CC3100_intISR, CC3100_intDSR, &isr_handle, &isr_object);
    cyg_interrupt_configure(CYGNUM_HAL_INTERRUPT_EXTI7, 0, true);
    cyg_interrupt_attach(isr_handle);
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EXTI7);
    HAL_READ_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(7), regval);
    regval |= CYGHWR_HAL_STM32_SYSCFG_EXTICRX_PORTD(7);
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(7), regval);
#endif

    }

    initialized = true;

    debug_printf("done!\n\r");
}

void stopWDT(void)
{
}

int registerInterruptHandler(P_EVENT_HANDLER InterruptHdl , void* pValue)
{
    pIrqEventHandler = InterruptHdl;

    return 0;
}

void CC3100_disable(void)
{
    hal_stm32_gpio_out(NHIB, 0);
}

void CC3100_enable(void)
{
    hal_stm32_gpio_out(NHIB, 1);
}

void CC3100_InterruptEnable(void)
{
#ifdef BOARD_EVAL
	cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EXTI14);
#endif
#ifdef BOARD_TR2
	cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EXTI7);
#endif
}

void CC3100_InterruptDisable(void)
{
#ifdef BOARD_EVAL
    cyg_interrupt_mask(CYGNUM_HAL_INTERRUPT_EXTI14);
#endif
#ifdef BOARD_TR2
    cyg_interrupt_mask(CYGNUM_HAL_INTERRUPT_EXTI7);
#endif
}

void MaskIntHdlr(void)
{
	IntIsMasked = TRUE;
}

void UnMaskIntHdlr(void)
{
	IntIsMasked = FALSE;
}

void Delay(unsigned long interval)
{
	cyg_thread_delay(interval);
}

cyg_uint32 CC3100_intISR(cyg_vector_t vector, cyg_addrword_t data)
{
#ifdef BOARD_EVAL
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_PR, CYGHWR_HAL_STM32_EXTI_BIT(14));
#endif
#ifdef BOARD_TR2
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_PR, CYGHWR_HAL_STM32_EXTI_BIT(7));
#endif


	return CYG_ISR_CALL_DSR;
}

void CC3100_intDSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{

	if(pIrqEventHandler)
    {
        pIrqEventHandler(0);
    }
}

void UART1_intHandler()
{
// 	unsigned long intStatus;
// 	intStatus =UARTIntStatus(UART1_BASE,0);
// 	UARTIntClear(UART1_BASE,intStatus);

// #ifdef SL_IF_TYPE_UART
// 	if((pIrqEventHandler != 0) && (IntIsMasked == FALSE))
// 	{
// 		pIrqEventHandler(0);
// 	}
// #endif
}
