//==========================================================================
//
//      i2c_stm32.c
//
//      I2C driver for STM32
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2010 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    Martin Rösch <roscmar@gmail.com>
// Contributors: 
// Date:         2010-10-28
// Description:  I2C bus driver for STM32
//              
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/system.h>
#include <pkgconf/devs_i2c_cortexm_stm32.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/io/i2c.h>
#include <cyg/io/i2c_stm32.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/drv_api.h>

//--------------------------------------------------------------------------
// Diagnostic support
#if CYGPKG_DEVS_I2C_CORTEXM_STM32_DEBUG_LEVEL > 0
   #define error_printf(args...) diag_printf(args)
   #define EVLOG(__x)
#else
   #define error_printf(args...)
   #define EVLOG(__x)
#endif
#if CYGPKG_DEVS_I2C_CORTEXM_STM32_DEBUG_LEVEL > 1
   #define debug_printf(args...) diag_printf(args)
   #define EVLOG(__x) (extra->i2c_evlog[extra->i2c_evlogidx++]=(__x))
#else
   #define debug_printf(args...)
   #define EVLOG(__x)
#endif

//--------------------------------------------------------------------------
// I2C bus instances
#ifdef CYGHWR_DEVS_I2C_CORTEXM_STM32_BUS1
CYG_STM32_I2C_BUS(i2c_bus1, CYGHWR_HAL_STM32_I2C1,
        CYGNUM_HAL_INTERRUPT_I2C1_EV, CYGNUM_HAL_INTERRUPT_I2C1_EE);
#endif

#ifdef CYGHWR_DEVS_I2C_CORTEXM_STM32_BUS2
CYG_STM32_I2C_BUS(i2c_bus2, CYGHWR_HAL_STM32_I2C2,
        CYGNUM_HAL_INTERRUPT_I2C2_EV, CYGNUM_HAL_INTERRUPT_I2C2_EE);
#endif

#ifdef CYGHWR_DEVS_I2C_CORTEXM_STM32_BUS3
CYG_STM32_I2C_BUS(i2c_bus3, CYGHWR_HAL_STM32_I2C3,
        CYGNUM_HAL_INTERRUPT_I2C3_EV, CYGNUM_HAL_INTERRUPT_I2C3_ER);
#endif

//--------------------------------------------------------------------------
// I2C event mask definitions for polling mode
#define I2C_MS_EV5 (((CYGHWR_HAL_STM32_I2C_SR2_MSL | CYGHWR_HAL_STM32_I2C_SR2_BUSY)<< 16) | CYGHWR_HAL_STM32_I2C_SR1_SB)
#define I2C_MS_RX_EV6 (((CYGHWR_HAL_STM32_I2C_SR2_MSL | CYGHWR_HAL_STM32_I2C_SR2_BUSY)<< 16) | CYGHWR_HAL_STM32_I2C_SR1_ADDR)
#define I2C_MS_RX_EV7 (((CYGHWR_HAL_STM32_I2C_SR2_MSL | CYGHWR_HAL_STM32_I2C_SR2_BUSY)<< 16) | CYGHWR_HAL_STM32_I2C_SR1_RxNE | CYGHWR_HAL_STM32_I2C_SR1_BTF)
#define I2C_MS_RX_EV7_1 (((CYGHWR_HAL_STM32_I2C_SR2_MSL | CYGHWR_HAL_STM32_I2C_SR2_BUSY)<< 16) | CYGHWR_HAL_STM32_I2C_SR1_RxNE)
#define I2C_MS_TX_EV6 (((CYGHWR_HAL_STM32_I2C_SR2_MSL | CYGHWR_HAL_STM32_I2C_SR2_BUSY | CYGHWR_HAL_STM32_I2C_SR2_TRA)<< 16) | CYGHWR_HAL_STM32_I2C_SR1_TxE | CYGHWR_HAL_STM32_I2C_SR1_ADDR)
#define I2C_MS_EV8_1 (((CYGHWR_HAL_STM32_I2C_SR2_MSL | CYGHWR_HAL_STM32_I2C_SR2_BUSY | CYGHWR_HAL_STM32_I2C_SR2_TRA)<< 16) | CYGHWR_HAL_STM32_I2C_SR1_TxE)
#define I2C_MS_EV8 (((CYGHWR_HAL_STM32_I2C_SR2_MSL | CYGHWR_HAL_STM32_I2C_SR2_BUSY | CYGHWR_HAL_STM32_I2C_SR2_TRA)<< 16) | CYGHWR_HAL_STM32_I2C_SR1_TxE | CYGHWR_HAL_STM32_I2C_SR1_BTF)
#define I2C_MS_EV8_2 (((CYGHWR_HAL_STM32_I2C_SR2_MSL | CYGHWR_HAL_STM32_I2C_SR2_BUSY | CYGHWR_HAL_STM32_I2C_SR2_TRA)<< 16) | CYGHWR_HAL_STM32_I2C_SR1_TxE | CYGHWR_HAL_STM32_I2C_SR1_BTF)


//--------------------------------------------------------------------------
// I2C macros for remap pins
#define stm32_i2c_scl( port_pin ) CYGHWR_HAL_STM32_GPIO( port_pin , ALTFN, 4, OPENDRAIN, PULLUP, 50MHZ)
#define stm32_i2c_sda( port_pin ) CYGHWR_HAL_STM32_GPIO( port_pin , ALTFN, 4, OPENDRAIN, PULLUP, 50MHZ)

//--------------------------------------------------------------------------
// Poll I2C status registers (SR2, SR1) until they are equal to a given event
// mask or until timeout.
inline static cyg_uint32 i2c_wait_for_event(cyg_uint32 event_mask, \
        CYG_ADDRESS base)
{
    volatile cyg_uint32 sr1 = 0, sr2 = 0, tmo = 0;
    do {
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, sr1);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, sr2);
        tmo++;
    } while(((sr2<<16 |sr1) != event_mask) && (tmo < 0x000FFFFF));
    return (sr2<<16 |sr1) ^ event_mask;
}

static cyg_bool i2c_wait_for_flag_set(CYG_ADDRESS addr, cyg_uint32 flag_mask)
{
    volatile cyg_uint32 reg = 0, tmo = 0;
    do {
        HAL_READ_UINT16(addr, reg);
        tmo++;
        if(tmo >= 0x000FFFFF)
        {
            error_printf("I2C: %s : %u (0x%p) = 0x%04x (mask 0x%04x)\r\n", __FUNCTION__, __LINE__, (void*)addr, reg, flag_mask);
            return false;
        }
    }while((reg & flag_mask) == 0);

    debug_printf("I2C: %s : %u reg = 0x%x\r\n", __FUNCTION__, __LINE__, reg);
    return true;
}

static cyg_bool i2c_wait_for_flag_reset(CYG_ADDRESS addr, cyg_uint32 flag_mask)
{
    volatile cyg_uint32 reg = 0, tmo = 0;
    do {
        HAL_READ_UINT16(addr, reg);
        tmo++;
        if(tmo >= 0x000FFFFF)
        {
            error_printf("I2C: %s : %u (0x%p) = 0x%04x (mask 0x%04x)\r\n", __FUNCTION__, __LINE__, (void*)addr, reg, flag_mask);
            return false;
        }
    }while((reg & flag_mask) != 0);

    debug_printf("I2C: %s : %u reg = 0x%x\r\n", __FUNCTION__, __LINE__, reg);
    return true;
}

static cyg_bool i2c_wait_on_addr_flag(CYG_ADDRESS base)
{
    volatile cyg_uint32 reg1 = 0, reg2 = 0, tmo = 0;
    
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg1);
    debug_printf("I2C: %s : %u  SR1 = 0x%x\r\n", __FUNCTION__, __LINE__, reg1);
    while(!(reg1 & CYGHWR_HAL_STM32_I2C_SR1_ADDR))
    {
        /* check if AF flag is SET */
        if(reg1 & CYGHWR_HAL_STM32_I2C_SR1_AF)
        {
            error_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
            /* Generate Stop */
            HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg2);
            reg2 |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
            HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg2);
            
            /* Clear AF Flag */
            HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg2);
            reg2 &= ~CYGHWR_HAL_STM32_I2C_SR1_AF;
            HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg2);

            return false;
        }
        tmo++;
        if(tmo >= 0x000FFFFF)
        {
            error_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
            return false;
        }
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg1);
    }

    return true;
}

static inline cyg_uint32 get_flag_state(CYG_ADDRESS reg, cyg_uint16 mask)
{
    cyg_uint16 val;
    HAL_READ_UINT16(reg, val);
    debug_printf("I2C: %s : %u (0x%p) = 0x%04x (mask 0x%04x)\r\n", __FUNCTION__, __LINE__, (void*)reg, val, mask);
    return ((val & mask) ? 1 : 0);
}

//--------------------------------------------------------------------------
// The event ISR does the actual work. It is not that much work to justify
// putting it in the DSR, and it is also not clear whether this would
// even work.
//
// The logic for extra->i2c_rxnak is commented out. It's purpose is to 
// follow the API and let the caller decide whether to NACK. However,
// when it is set to false, the STOP fails, and even a reset of the target
// does not seem to clean things up. Only a power cycle cleared the problem.
// Therefore, it is in the code for completeness, but disabled so that the
// caller can't shoot themselves in the foot.
static cyg_uint32 stm32_i2c_ev_isr(cyg_vector_t vec, cyg_addrword_t data)
{
    static cyg_stm32_i2c_extra* extra = NULL;
    static CYG_ADDRESS base = 0;
    cyg_bool call_dsr = false;
    cyg_uint32 tmp1 = 0, tmp2 = 0, tmp3 = 0, tmp4 = 0;
    cyg_uint16 reg;

    extra = (cyg_stm32_i2c_extra*)data;
    base = extra->i2c_base;

    /* Master mode selected */
    if(get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR2, CYGHWR_HAL_STM32_I2C_SR2_MSL))
    {
        /* I2C in mode Transmitter -----------------------------------------------*/
        if(get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR2, CYGHWR_HAL_STM32_I2C_SR2_TRA))
        {
            tmp1 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_TxE);
            tmp2 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_CR2, CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN);
            tmp3 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_BTF);
            tmp4 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_CR2, CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN);
            /* TXE set and BTF reset -----------------------------------------------*/
            if(tmp1 && tmp2 && !tmp3)
            {
                EVLOG('a');
                // I2C_MasterTransmit_TXE();
                HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_txbuf));
                extra->i2c_txbuf++;
                extra->i2c_txleft--;
                if(extra->i2c_txleft == 0)
                {
                    EVLOG('b');
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
                }
            }
            /* BTF set -------------------------------------------------------------*/
            else if(tmp3 && tmp4)
            {
                // I2C_MasterTransmit_BTF();
                if(extra->i2c_txleft > 0)
                {
                    EVLOG('c');
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_txbuf));
                    extra->i2c_txbuf++;
                    extra->i2c_txleft--;
                }
                else
                {
                    EVLOG('d');
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN;
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN;
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITERREN;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);

                    if(extra->i2c_stop)
                    {
                        EVLOG('e');
                        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                        reg |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
                        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                        /* Wait until BUSY flag is reset */
                        i2c_wait_for_flag_reset(base + CYGHWR_HAL_STM32_I2C_SR2, CYGHWR_HAL_STM32_I2C_SR2_BUSY);
                    }
                    call_dsr = true;
                }
            }
        }
        /* I2C in mode Receiver --------------------------------------------------*/
        else
        {
            tmp1 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_RxNE);
            tmp2 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_CR2, CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN);
            tmp3 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_BTF);
            tmp4 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_CR2, CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN);
            /* RXNE set and BTF reset -----------------------------------------------*/
            if(tmp1 && tmp2 && !tmp3)
            {
                // I2C_MasterReceive_RXNE();
                if(extra->i2c_rxleft > 3)
                {
                    EVLOG('f');
                    /* Read data from DR */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_rxbuf));
                    extra->i2c_rxbuf++;
                    extra->i2c_rxleft--;
                }
                else if((extra->i2c_rxleft == 2) || (extra->i2c_rxleft == 3))
                {
                    EVLOG('g');
                    /* Disable BUF interrupt */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
                }
                else
                {
                    EVLOG('h');
                    /* Disable EVT, BUF and ERR interrupt */                    
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN;
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN;
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITERREN;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
                    /* Read data from DR */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_rxbuf));
                    extra->i2c_rxbuf++;
                    extra->i2c_rxleft--;
                    /* Wait until BUSY flag is reset */
                    i2c_wait_for_flag_reset(base + CYGHWR_HAL_STM32_I2C_SR2, CYGHWR_HAL_STM32_I2C_SR2_BUSY);
                    /* Disable Pos */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR1_POS;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    // if(extra->i2c_stop)
                    // {
                    //     HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    //     reg |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
                    //     HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    // }
                    call_dsr = true;
                }
            }
            /* BTF set -------------------------------------------------------------*/
            else if(tmp3 && tmp4)
            {
                // I2C_MasterReceive_BTF();
                if(extra->i2c_rxleft == 3)
                {
                    EVLOG('i');
                    /* Disable Acknowledge */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR1_ACK;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    /* Read data from DR */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_rxbuf));
                    extra->i2c_rxbuf++;
                    extra->i2c_rxleft--;
                }
                else if(extra->i2c_rxleft == 2)
                {
                    EVLOG('j');
                    /* Generate Stop */
                    if(extra->i2c_stop)
                    {
                        EVLOG('k');
                        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                        reg |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
                        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    }
                    /* Read data from DR */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_rxbuf));
                    extra->i2c_rxbuf++;
                    extra->i2c_rxleft--;
                    /* Read data from DR */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_rxbuf));
                    extra->i2c_rxbuf++;
                    extra->i2c_rxleft--;
                    /* Disable EVT and ERR interrupt */                    
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN;
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITERREN;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
                    /* Wait until BUSY flag is reset */
                    i2c_wait_for_flag_reset(base + CYGHWR_HAL_STM32_I2C_SR2, CYGHWR_HAL_STM32_I2C_SR2_BUSY);
                    /* Disable Pos */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    reg &= ~CYGHWR_HAL_STM32_I2C_CR1_POS;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    call_dsr = true;
                }
                else
                {
                    EVLOG('l');
                    /* Read data from DR */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_rxbuf));
                    extra->i2c_rxbuf++;
                    extra->i2c_rxleft--;
                }
            }
        }
    }
    /* Slave mode selected */
    else
    {
        tmp1 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_ADDR);
        tmp2 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_CR2, CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN);
        tmp3 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_STOPF);
        tmp4 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR2, CYGHWR_HAL_STM32_I2C_SR2_TRA);
        /* ADDR set --------------------------------------------------------------*/
        if(tmp1 && tmp2)
        {
            // I2C_Slave_ADDR();
            EVLOG('m');
            HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
            HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);
            reg = 0;

        }
        /* STOPF set --------------------------------------------------------------*/
        else if(tmp3 && tmp2)
        {
            // I2C_Slave_STOPF();
             error_printf("I2C_Slave_STOPF\r\n");
        }
        /* I2C in mode Transmitter -----------------------------------------------*/
        else if(tmp4)
        {
            tmp1 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_TxE);
            tmp2 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_CR2, CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN);
            tmp3 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_BTF);
            tmp4 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_CR2, CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN);
            /* TXE set and BTF reset -----------------------------------------------*/
            if(tmp1 && tmp2 && !tmp3)
            {
                // I2C_SlaveTransmit_TXE();
                error_printf("I2C_SlaveTransmit_TXE\r\n");
            }
            /* BTF set -------------------------------------------------------------*/
            else if(tmp3 && tmp4)
            {
                // I2C_SlaveTransmit_BTF();
                error_printf("I2C_SlaveTransmit_BTF\r\n");
            }
        }
        /* I2C in mode Receiver --------------------------------------------------*/
        else
        {
            tmp1 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_RxNE);
            tmp2 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_CR2, CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN);
            tmp3 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_BTF);
            tmp4 = get_flag_state(base + CYGHWR_HAL_STM32_I2C_CR2, CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN);
            /* RXNE set and BTF reset ----------------------------------------------*/
            if(tmp1 && tmp2 && !tmp3)
            {
                EVLOG('n');
                // I2C_SlaveReceive_RXNE();
                if(extra->i2c_rxleft != 0)
                {
                    /* Read data from DR */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_rxbuf));
                    extra->i2c_rxbuf++;
                    extra->i2c_rxleft--;
                }
            }
            /* BTF set -------------------------------------------------------------*/
            else if(tmp3 && tmp4)
            {
                EVLOG('o');
                // I2C_SlaveReceive_BTF();
                if(extra->i2c_rxleft != 0)
                {
                    /* Read data from DR */
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, *(extra->i2c_rxbuf));
                    extra->i2c_rxbuf++;
                    extra->i2c_rxleft--;
                }
            }
        }
    }
    // Acknowledge interrupt
    cyg_drv_interrupt_acknowledge(vec);

#if CYGPKG_DEVS_I2C_CORTEXM_STM32_DEBUG_LEVEL > 0
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    debug_printf("I2C: %s : %u CR1 = 0x%04x\n", __FUNCTION__, __LINE__, reg);
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_CR2, reg);
    debug_printf("I2C: %s : %u CR2 = 0x%04x\n", __FUNCTION__, __LINE__, reg);
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_SR1, reg);
    debug_printf("I2C: %s : %u SR1 = 0x%04x\n", __FUNCTION__, __LINE__, reg);
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_SR2, reg);
    debug_printf("I2C: %s : %u SR2 = 0x%04x\n", __FUNCTION__, __LINE__, reg);
#endif
    // We need to call the DSR only if there is really something to signal
    if (call_dsr)
    {
        EVLOG('x');
        return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;
    }
    else
    {
        EVLOG('y');
        return CYG_ISR_HANDLED;
    }
}

//--------------------------------------------------------------------------
// The error ISR wakes up the driver in case an error occurred. Both status
// registers are stored in the i2c_status field (high and low word for SR2
// and SR1 respectively) of the driver data.
static cyg_uint32 stm32_i2c_err_isr(cyg_vector_t vec, cyg_addrword_t data)
{
    static cyg_stm32_i2c_extra* extra = NULL;
    CYG_ADDRESS base;
    cyg_uint16  sr1, sr2, cr1;

    error_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
    extra = (cyg_stm32_i2c_extra*)data;
    base = extra->i2c_base;
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, sr1);
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, sr2);
    extra->i2c_status = (sr2 << 16) | sr1;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, 0);  //clear it
    // Stop the transfer
    if((sr2 & CYGHWR_HAL_STM32_I2C_SR2_BUSY) > 0)
    {
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, cr1);
        cr1 |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, cr1);
    }
    // Acknowledge interrupt
    cyg_drv_interrupt_acknowledge(vec);
    return CYG_ISR_HANDLED | CYG_ISR_CALL_DSR;
}

//--------------------------------------------------------------------------
// The event DSR just wakes up the driver code.
static void stm32_i2c_ev_dsr(cyg_vector_t vec, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_stm32_i2c_extra* extra = (cyg_stm32_i2c_extra*)data;
    CYG_ASSERT(extra->i2c_wait.mutex->locked == 0, "i2c_wait locked");
    cyg_drv_cond_signal(&extra->i2c_wait);
}

//--------------------------------------------------------------------------
// The error DSR just wakes up the driver code.
static void stm32_i2c_err_dsr(cyg_vector_t vec, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_stm32_i2c_extra* extra = (cyg_stm32_i2c_extra*)data;
    error_printf("I2C error: 0x%x\n", extra->i2c_status);
    cyg_drv_cond_signal(&extra->i2c_wait);
}

//--------------------------------------------------------------------------
// Transmits a buffer to a device in interrupt mode
cyg_uint32 cyg_stm32_i2c_tx_int(const cyg_i2c_device *dev,
                            cyg_bool              send_start,
                            const cyg_uint8      *tx_data,
                            cyg_uint32            count,
                            cyg_bool              send_stop)
{
    cyg_stm32_i2c_extra* extra = (cyg_stm32_i2c_extra*)dev->i2c_bus->i2c_extra;
    CYG_ADDRESS base = extra->i2c_base;
    cyg_uint16 reg;
    extra->i2c_addr  = dev->i2c_address << 1;
    extra->i2c_txleft = count;
    extra->i2c_txbuf = tx_data;
    extra->i2c_stop = send_stop;
#if CYGPKG_DEVS_I2C_CORTEXM_STM32_DEBUG_LEVEL > 1
    cyg_uint16 i;
    extra->i2c_evlogidx = 0;
#endif
    debug_printf("I2C: %s : %u count = %u\n", __FUNCTION__, __LINE__, count);
    while(!cyg_drv_mutex_lock(&extra->i2c_lock));
    cyg_drv_dsr_lock();
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);
    if(reg & CYGHWR_HAL_STM32_I2C_SR2_BUSY)
    {
        cyg_drv_dsr_unlock();
        cyg_drv_mutex_unlock(&extra->i2c_lock);
        error_printf("I2C: %s return on %u (reg = 0x%x)\r\n", __FUNCTION__, __LINE__, reg);
        return 0;
    }

    extra->i2c_state = I2C_STATE_BUSY;

    if(send_start) 
    {
        debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg |= CYGHWR_HAL_STM32_I2C_CR1_START;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        /* Wait until SB flag is set */
        if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_SB))
        {
            cyg_drv_dsr_unlock();
            cyg_drv_mutex_unlock(&extra->i2c_lock);
            error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
            return 0;
        }
    }

    /* Send slave address */
    debug_printf("I2C: %s : %u addr = 0x%x\r\n", __FUNCTION__, __LINE__, extra->i2c_addr);
    HAL_WRITE_UINT8(base + CYGHWR_HAL_STM32_I2C_DR, extra->i2c_addr);

    /* Wait until ADDR flag is set */
    if(!i2c_wait_on_addr_flag(base))
    {
        cyg_drv_dsr_unlock();
        cyg_drv_mutex_unlock(&extra->i2c_lock);
        error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
        return 0;
    }

    /* Clear ADDR flag */
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);

    cyg_drv_interrupt_unmask(extra->i2c_ev_vec);
    cyg_drv_interrupt_unmask(extra->i2c_err_vec);

    // Enable I2C bus interrupts
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
    reg |= CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN;
    reg |= CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN;
    reg |= CYGHWR_HAL_STM32_I2C_CR2_ITERREN;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);

    // The ISR will do most of the work, and the DSR will signal when an
    // error occurred or the transfer finished
    cyg_drv_cond_wait(&extra->i2c_wait);
    debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);

    cyg_drv_interrupt_mask(extra->i2c_ev_vec);
    cyg_drv_interrupt_mask(extra->i2c_err_vec);
    cyg_drv_mutex_unlock(&extra->i2c_lock);
    cyg_drv_dsr_unlock();

    // Disable I2C bus interrupts
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN;
    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN;
    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITERREN;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);

#if CYGPKG_DEVS_I2C_CORTEXM_STM32_DEBUG_LEVEL > 1
    debug_printf("LOG: ");
    for(i=0; i< extra->i2c_evlogidx; i++)
        debug_printf("%c", extra->i2c_evlog[i]);
    debug_printf("\n");
#endif
    if(send_stop)
    {
        extra->i2c_state = I2C_STATE_IDLE;
    }
    extra->i2c_addr  = 0;
    extra->i2c_txbuf = NULL;
    return count - extra->i2c_txleft;
}

//--------------------------------------------------------------------------
// Receive into a buffer from a device in interrupt mode
cyg_uint32 cyg_stm32_i2c_rx_int(const cyg_i2c_device *dev,
                            cyg_bool              send_start,
                            cyg_uint8            *rx_data,
                            cyg_uint32            count,
                            cyg_bool              send_nak,
                            cyg_bool              send_stop)
{
    cyg_stm32_i2c_extra* extra =
                           (cyg_stm32_i2c_extra*)dev->i2c_bus->i2c_extra;
    CYG_ADDRESS base = extra->i2c_base;
    cyg_uint16 reg;
    extra->i2c_addr  = (dev->i2c_address << 1) | 1;
    extra->i2c_rxleft = count;
    extra->i2c_rxbuf = rx_data;
    extra->i2c_rxnak = send_nak;
    extra->i2c_stop = send_stop;
#if CYGPKG_DEVS_I2C_CORTEXM_STM32_DEBUG_LEVEL > 1
    cyg_uint16 i;
    extra->i2c_evlogidx = 0;
#endif
    debug_printf("I2C: %s : %u count = %u\n", __FUNCTION__, __LINE__, count);
    cyg_drv_mutex_lock(&extra->i2c_lock);
    cyg_drv_dsr_lock();

    if(extra->i2c_state == I2C_STATE_IDLE)
    {
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);
        if(reg & CYGHWR_HAL_STM32_I2C_SR2_BUSY)
        {
            cyg_drv_dsr_unlock();
            error_printf("I2C: %s return on %u (reg = 0x%x)\r\n", __FUNCTION__, __LINE__, reg);
            return 0;
        }
    }

    /* Enable Acknowledge */
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    reg |= CYGHWR_HAL_STM32_I2C_CR1_ACK;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

    /* Generate Start */
    if(send_start)
    {
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg |= CYGHWR_HAL_STM32_I2C_CR1_START;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        /* Wait until SB flag is set */
        if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_SB))
        {
            cyg_drv_dsr_unlock();
            error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
            return 0;
        }
    }

    /* Send slave address */
    HAL_WRITE_UINT8(base + CYGHWR_HAL_STM32_I2C_DR, extra->i2c_addr);

    /* Wait until ADDR flag is set */
    if(!i2c_wait_on_addr_flag(base))
    {
        cyg_drv_dsr_unlock();
        error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if(extra->i2c_rxleft == 1)
    {
        debug_printf("I2C: %s : %u\n", __FUNCTION__, __LINE__);
        /* Disable Acknowledge */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg &= ~CYGHWR_HAL_STM32_I2C_CR1_ACK;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

        /* Clear ADDR flag */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);

        /* Generate Stop */
        if(send_stop)
        {
            HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
            reg |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
            HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        }
    }
    else if(extra->i2c_rxleft == 2)
    {
        debug_printf("I2C: %s : %u\n", __FUNCTION__, __LINE__);
        /* Disable Acknowledge */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg &= ~CYGHWR_HAL_STM32_I2C_CR1_ACK;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

        /* Enable Pos */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg |= CYGHWR_HAL_STM32_I2C_CR1_POS;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

        /* Clear ADDR flag */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);
    }
    else
    {
        debug_printf("I2C: %s : %u\n", __FUNCTION__, __LINE__);
        /* Enable Acknowledge */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg |= CYGHWR_HAL_STM32_I2C_CR1_ACK;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

        /* Clear ADDR flag */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);
    }

    cyg_drv_interrupt_unmask(extra->i2c_ev_vec);
    cyg_drv_interrupt_unmask(extra->i2c_err_vec);

    /* Enable EVT, BUF and ERR interrupt */
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
    reg |= CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN;
    reg |= CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN;
    reg |= CYGHWR_HAL_STM32_I2C_CR2_ITERREN;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);

    // The ISR will do most of the work, and the DSR will signal when an
    // error occurred or the transfer finished
    cyg_drv_cond_wait(&extra->i2c_wait);
    debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);

    cyg_drv_interrupt_mask(extra->i2c_ev_vec);
    cyg_drv_interrupt_mask(extra->i2c_err_vec);
    cyg_drv_dsr_unlock();
    cyg_drv_mutex_unlock(&extra->i2c_lock);

    // Disable I2C bus interrupts
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);
    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITEVTEN;
    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITBUFEN;
    reg &= ~CYGHWR_HAL_STM32_I2C_CR2_ITERREN;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR2, reg);

    // Enable Acknowledgment to be ready for another reception
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    reg |= CYGHWR_HAL_STM32_I2C_CR1_ACK;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
#if CYGPKG_DEVS_I2C_CORTEXM_STM32_DEBUG_LEVEL > 1
    debug_printf("LOG: ");
    for(i=0; i< extra->i2c_evlogidx; i++)
        debug_printf("%c", extra->i2c_evlog[i]);
    debug_printf("\n");
#endif
    extra->i2c_addr  = 0;
    extra->i2c_rxbuf = NULL;

    debug_printf("I2C: %s : %u\n", __FUNCTION__, __LINE__);
    return (count - extra->i2c_rxleft);
}

//--------------------------------------------------------------------------
// Transmit a buffer to a device in polling mode
cyg_uint32 cyg_stm32_i2c_tx_poll(const cyg_i2c_device *dev,
                            cyg_bool              send_start,
                            const cyg_uint8      *tx_data,
                            cyg_uint32            count,
                            cyg_bool              send_stop)
{
    cyg_stm32_i2c_extra* extra = (cyg_stm32_i2c_extra*)dev->i2c_bus->i2c_extra;
    CYG_ADDRESS base = extra->i2c_base;
    cyg_uint16 reg;
    extra->i2c_addr  = dev->i2c_address << 1;
    extra->i2c_txleft = count;
    extra->i2c_txbuf = tx_data;

    cyg_drv_dsr_lock();

    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);
    if(reg & CYGHWR_HAL_STM32_I2C_SR2_BUSY)
    {
        cyg_drv_dsr_unlock();
        error_printf("I2C: %s return on %u (reg = 0x%x)\r\n", __FUNCTION__, __LINE__, reg);
        return 0;
    }

    if(send_start) 
    {
        debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg |= CYGHWR_HAL_STM32_I2C_CR1_START;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    }

    extra->i2c_state = I2C_STATE_BUSY;

    /* Wait until SB flag is set */
    if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_SB))
    {
        cyg_drv_dsr_unlock();
        error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
        return 0;
    }

    /* Send slave address */
    debug_printf("I2C: %s : %u addr = 0x%x\r\n", __FUNCTION__, __LINE__, extra->i2c_addr);
    HAL_WRITE_UINT8(base + CYGHWR_HAL_STM32_I2C_DR, extra->i2c_addr);

    /* Wait until ADDR flag is set */
    if(!i2c_wait_on_addr_flag(base))
    {
        cyg_drv_dsr_unlock();
        error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
        return 0;
    }

    /* Clear ADDR flag */
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);

    while(extra->i2c_txleft > 0)
    {
        /* Wait until TXE flag is set */
        if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_TxE))
        {
            cyg_drv_dsr_unlock();
            error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
            return 0;
        }

        /* Write data to DR */
        debug_printf("I2C: %s : %u data = 0x%x\r\n", __FUNCTION__, __LINE__, *extra->i2c_txbuf);
        HAL_WRITE_UINT8(base + CYGHWR_HAL_STM32_I2C_DR, *extra->i2c_txbuf++);
        extra->i2c_txleft--;

        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
        if((reg & CYGHWR_HAL_STM32_I2C_SR1_BTF) && (count != 0))
        {
            /* Write data to DR */
            debug_printf("I2C: %s : %u data = 0x%x\r\n", __FUNCTION__, __LINE__, *extra->i2c_txbuf);
            HAL_WRITE_UINT8(base + CYGHWR_HAL_STM32_I2C_DR, (*extra->i2c_txbuf++));
            extra->i2c_txleft--;
        }
    }

    /* Wait until TXE flag is set */
    if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_TxE))
    {
        cyg_drv_dsr_unlock();
        error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if(send_stop)
    {
        /* Generate Stop */
        debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        /* Wait until BUSY flag is reset */
        if(!i2c_wait_for_flag_reset(base + CYGHWR_HAL_STM32_I2C_SR2, CYGHWR_HAL_STM32_I2C_SR2_BUSY))
        {
            cyg_drv_dsr_unlock();
            error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
            return 0;
        }
        extra->i2c_state = I2C_STATE_IDLE;
    }
    // else
    // {
    //     debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
    //     HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    //     reg |= CYGHWR_HAL_STM32_I2C_CR1_START;
    //     HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    // }


    /* Process Unlocked */
    cyg_drv_dsr_unlock();

    return (count - extra->i2c_txleft);
}

//--------------------------------------------------------------------------
// Receive into a buffer from a device in polling mode
//
// The logic for extra->i2c_rxnak is commented out. It's purpose is to 
// follow the API and let the caller decide whether to NACK. However,
// when it is set to false, the STOP fails, and even a reset of the target
// does not seem to clean things up. Only a power cycle cleared the problem.
// Therefore, it is in the code for completeness, but disabled so that the
// caller can't shoot themselves in the foot.
cyg_uint32 cyg_stm32_i2c_rx_poll(const cyg_i2c_device *dev,
                            cyg_bool              send_start,
                            cyg_uint8            *rx_data,
                            cyg_uint32            count,
                            cyg_bool              send_nak,
                            cyg_bool              send_stop)
{
    cyg_stm32_i2c_extra* extra = (cyg_stm32_i2c_extra*)dev->i2c_bus->i2c_extra;
    CYG_ADDRESS base = extra->i2c_base;
    cyg_uint16 reg;
    extra->i2c_addr  = (dev->i2c_address << 1) | 0x01;
    extra->i2c_rxleft = count;
    extra->i2c_rxbuf = rx_data;
    extra->i2c_rxnak = send_nak;

    if(count == 0)
        return 0;

    cyg_drv_dsr_lock();

    if(extra->i2c_state == I2C_STATE_IDLE)
    {
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);
        if(reg & CYGHWR_HAL_STM32_I2C_SR2_BUSY)
        {
            cyg_drv_dsr_unlock();
            error_printf("I2C: %s return on %u (reg = 0x%x)\r\n", __FUNCTION__, __LINE__, reg);
            return 0;
        }
    }

    /* Enable Acknowledge */
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    reg |= CYGHWR_HAL_STM32_I2C_CR1_ACK;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

    /* Generate Start */
    if(send_start)
    {
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg |= CYGHWR_HAL_STM32_I2C_CR1_START;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    }

    /* Wait until SB flag is set */
    if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_SB))
    {
        cyg_drv_dsr_unlock();
        error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
        return 0;
    }

    /* Send slave address */
    HAL_WRITE_UINT8(base + CYGHWR_HAL_STM32_I2C_DR, extra->i2c_addr);

    /* Wait until ADDR flag is set */
    if(!i2c_wait_on_addr_flag(base))
    {
        cyg_drv_dsr_unlock();
        error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if(count == 1)
    {
        /* Disable Acknowledge */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
            reg &= ~CYGHWR_HAL_STM32_I2C_CR1_ACK;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

        /* Clear ADDR flag */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);

        /* Generate Stop */
        if(send_stop)
        {
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
            reg |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
            extra->i2c_state = I2C_STATE_IDLE;
            debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
        }
        debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
    }
    else if(count == 2)
        {
        /* Disable Acknowledge */
                HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg &= ~CYGHWR_HAL_STM32_I2C_CR1_ACK;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

        /* Enable Pos */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg |= CYGHWR_HAL_STM32_I2C_CR1_POS;
        HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

        /* Clear ADDR flag */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);
        debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
    }
    else
    {
        /* Enable Acknowledge */
            HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
        reg |= CYGHWR_HAL_STM32_I2C_CR1_ACK;
            HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

        /* Clear ADDR flag */
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR2, reg);
        debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
    }

    while(extra->i2c_rxleft > 0)
    {
        if(extra->i2c_rxleft <= 3)
        {
            /* One byte */
            if(extra->i2c_rxleft == 1)
            {
                /* Wait until RXNE flag is set */
                if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_RxNE))
                {
                    cyg_drv_dsr_unlock();
                    error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
                    return 0;
                }

                /* Read data from DR */
                HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, (*extra->i2c_rxbuf++));
                extra->i2c_rxleft--;
                debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
            }
            /* Two bytes */
            else if(extra->i2c_rxleft == 2)
            {
                /* Wait until BTF flag is set */
                if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_BTF))
                {
                    cyg_drv_dsr_unlock();
                    error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
                    return 0;
                }
                /* Generate Stop */
                if(send_stop)
                {
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    reg |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    extra->i2c_state = I2C_STATE_IDLE;
                    debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
                }

                /* Read data from DR */
                HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, (*extra->i2c_rxbuf++));
                extra->i2c_rxleft--;

                /* Read data from DR */
                HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, (*extra->i2c_rxbuf++));
                extra->i2c_rxleft--;
                debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
            }
            /* 3 Last bytes */
            else
            {
                /* Wait until BTF flag is set */
                if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_BTF))
                {
                    cyg_drv_dsr_unlock();
                    error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
                    return 0;
                }

                /* Disable Acknowledge */
                HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                reg &= ~CYGHWR_HAL_STM32_I2C_CR1_ACK;
                HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

                /* Read data from DR */
                HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, (*extra->i2c_rxbuf++));
                extra->i2c_rxleft--;

                /* Wait until BTF flag is set */
                if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_BTF))
                {
                    cyg_drv_dsr_unlock();
                    error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
                    return 0;
                }

                /* Generate Stop */
                if(send_stop)
                {
                    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    reg |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
                    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
                    extra->i2c_state = I2C_STATE_IDLE;
                    debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
                }

                /* Read data from DR */
                HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, (*extra->i2c_rxbuf++));
                extra->i2c_rxleft--;

                /* Read data from DR */
                HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, (*extra->i2c_rxbuf++));
                extra->i2c_rxleft--;
                debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
            }
        }
        else
        {
            /* Wait until RXNE flag is set */
            if(!i2c_wait_for_flag_set(base + CYGHWR_HAL_STM32_I2C_SR1, CYGHWR_HAL_STM32_I2C_SR1_RxNE))
            {
                cyg_drv_dsr_unlock();
                error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
                return 0;
            }

            /* Read data from DR */
            HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, (*extra->i2c_rxbuf++));
            extra->i2c_rxleft--;

            HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_SR1, reg);
            if(reg & CYGHWR_HAL_STM32_I2C_SR1_BTF)
            {
                /* Read data from DR */
                HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_DR, (*extra->i2c_rxbuf++));
                extra->i2c_rxleft--;
            }
            debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
        }
    }

    /* Disable Pos */
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    reg &= ~CYGHWR_HAL_STM32_I2C_CR1_POS;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);

    debug_printf("I2C: %s : %u\r\n", __FUNCTION__, __LINE__);
    /* Wait until BUSY flag is reset */
    if(!i2c_wait_for_flag_reset(base + CYGHWR_HAL_STM32_I2C_SR2, CYGHWR_HAL_STM32_I2C_SR2_BUSY))
    {
        cyg_drv_dsr_unlock();
        error_printf("I2C: %s return on %u\r\n", __FUNCTION__, __LINE__);
        return 0;
    }

    /* Process Unlocked */
    cyg_drv_dsr_unlock();

    return (count - extra->i2c_rxleft);
}

//--------------------------------------------------------------------------
//  Generate a STOP condtition
void cyg_stm32_i2c_stop(const cyg_i2c_device *dev)
{
    cyg_stm32_i2c_extra* extra =
                           (cyg_stm32_i2c_extra*)dev->i2c_bus->i2c_extra;
    CYG_ADDRESS base = extra->i2c_base;
    cyg_uint16 reg;
    extra = extra; // Avoid compiler warning in case of singleton
    // Set stop condition
    HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    reg |= CYGHWR_HAL_STM32_I2C_CR1_STOP;
    HAL_WRITE_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    // Make sure that the STOP bit is cleared by hardware
    do
    {
        HAL_READ_UINT16(base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    } while((reg & CYGHWR_HAL_STM32_I2C_CR1_STOP) != 0);
    debug_printf("I2C: Stop condition generated.\n");
}

//--------------------------------------------------------------------------
// This function is called from the generic I2C infrastructure to initialize
// a bus device. It should perform any work needed to start up the device.
void cyg_stm32_i2c_init(struct cyg_i2c_bus *bus)
{
    cyg_stm32_i2c_extra* extra = (cyg_stm32_i2c_extra*)bus->i2c_extra;
    extern cyg_uint32 hal_stm32_pclk1;
    cyg_uint32 pinspec_scl, pinspec_sda; // Pin configurations
    cyg_uint32 bus_freq = 100000; // I2C bus frequency in Hz
    cyg_uint8  duty = 0; // duty cycle for fast mode (0 = 2, 1 = 16/9)
    cyg_uint8  smbus = 0; // smbus mode (0 = I2C, 1 = SMBUS)
    cyg_uint8  bus_addr = 0x01; // I2C bus address
    CYG_ADDRESS base;
    cyg_uint16 reg;
    cyg_uint16 tmp;
    cyg_uint32 rcc;

    // Assuming the I2C peripheral is disabled as this is called after reset.

    // Setup GPIO Pins
    // The built in assumptions are that the peripheral clock must be started for 
    // the reset to work and that reset works when the peripheral is disabled.
    if(extra->i2c_base == CYGHWR_HAL_STM32_I2C1)
    {
#ifdef CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS1_BUSFREQ
        bus_freq = CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS1_BUSFREQ;
#endif
#ifdef CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS1_DUTY_CYCLE_16TO9
        duty = 1;
#endif
#ifdef CYGSEM_DEVS_I2C_CORTEXM_STM32_BUS1_MODE
        smbus = 1;
#endif
#ifdef CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS1_ADDRESS
        bus_addr = CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS1_ADDRESS;
#endif

#if defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_F1)
#ifdef CYGSEM_DEVS_I2C_CORTEXM_STM32_BUS1_REMAP
        base = CYGHWR_HAL_STM32_AFIO;
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_AFIO_MAPR, rcc);
        // Remap I2C1 due to conflict with FSMCs NADV signal
        rcc |= CYGHWR_HAL_STM32_AFIO_MAPR_I2C1_RMP;
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_AFIO_MAPR, rcc);
        pinspec_scl = CYGHWR_HAL_STM32_GPIO(B, 8, OUT_50MHZ, ALT_OPENDRAIN);
        pinspec_sda = CYGHWR_HAL_STM32_GPIO(B, 9, OUT_50MHZ, ALT_OPENDRAIN);
#else
        pinspec_scl = CYGHWR_HAL_STM32_GPIO(B, 6, OUT_50MHZ, ALT_OPENDRAIN);
        pinspec_sda = CYGHWR_HAL_STM32_GPIO(B, 7, OUT_50MHZ, ALT_OPENDRAIN);
#endif
#elif defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_HIPERFORMANCE)
#ifdef CYGHWR_HAL_STM32_I2C1_REMAP
	pinspec_scl = stm32_i2c_scl( CYGHWR_HAL_STM32_I2C1_SCL );
	pinspec_sda = stm32_i2c_sda( CYGHWR_HAL_STM32_I2C1_SDA );
#else
	pinspec_scl = CYGHWR_HAL_STM32_GPIO(B, 8, ALTFN, 4, OPENDRAIN, PULLUP, 50MHZ);
	pinspec_sda = CYGHWR_HAL_STM32_GPIO(B, 9, ALTFN, 4, OPENDRAIN, PULLUP, 50MHZ);
#endif
#endif

#ifdef CYGSEM_DEVS_I2C_CORTEXM_STM32_BUS1_MODE_POLL
        // register functions
        bus->i2c_tx_fn = &cyg_stm32_i2c_tx_poll;
        bus->i2c_rx_fn = &cyg_stm32_i2c_rx_poll;
#endif
#ifdef CYGSEM_DEVS_I2C_CORTEXM_STM32_BUS1_MODE_INT
        // Initialize i2c event interrupt
        cyg_drv_interrupt_create(extra->i2c_ev_vec,
                CYGINT_DEVS_I2C_CORTEXM_STM32_BUS1_MODE_INT_EV_PRI,
                (cyg_addrword_t) extra,
                &stm32_i2c_ev_isr,
                &stm32_i2c_ev_dsr,
                &(extra->i2c_ev_interrupt_handle),
                &(extra->i2c_ev_interrupt_data));
        cyg_drv_interrupt_attach(extra->i2c_ev_interrupt_handle);

        // Initialize i2c error interrupt
        cyg_drv_interrupt_create(extra->i2c_err_vec,
                CYGINT_DEVS_I2C_CORTEXM_STM32_BUS1_MODE_INT_EE_PRI,
                (cyg_addrword_t) extra,
                &stm32_i2c_err_isr,
                &stm32_i2c_err_dsr,
                &(extra->i2c_err_interrupt_handle),
                &(extra->i2c_err_interrupt_data));
        cyg_drv_interrupt_attach(extra->i2c_err_interrupt_handle);
        // register functions
        bus->i2c_tx_fn = &cyg_stm32_i2c_tx_int;
        bus->i2c_rx_fn = &cyg_stm32_i2c_rx_int;
#endif
        // Enable peripheral clock
        base = CYGHWR_HAL_STM32_RCC;
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1ENR, rcc);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB1ENR_I2C1);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1ENR, rcc);
#if defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_F1)
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB2ENR, rcc);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB2ENR_IOPB);        
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB2ENR_AFIO);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB2ENR, rcc);
#elif defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_HIPERFORMANCE)
        // This register exists, but not these bits.
#endif

        // Reset peripheral
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1RSTR, rcc);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB1ENR_I2C1);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1RSTR, rcc);
        rcc &= ~BIT_(CYGHWR_HAL_STM32_RCC_APB1ENR_I2C1);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1RSTR, rcc);
    }
    else if(extra->i2c_base == CYGHWR_HAL_STM32_I2C2)
    {
#ifdef CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS2_BUSFREQ
        bus_freq = CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS2_BUSFREQ;
#endif
#ifdef CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS2_DUTY_CYCLE_16TO9
        duty = 1;
#endif
#ifdef CYGSEM_DEVS_I2C_CORTEXM_STM32_BUS2_MODE
        smbus = 1;
#endif
#ifdef CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS2_ADDRESS
        bus_addr = CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS2_ADDRESS;
#endif
#if defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_F1)
        pinspec_scl = CYGHWR_HAL_STM32_GPIO(B, 10, OUT_50MHZ, ALT_OPENDRAIN);
        pinspec_sda = CYGHWR_HAL_STM32_GPIO(B, 11, OUT_50MHZ, ALT_OPENDRAIN);
#elif defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_HIPERFORMANCE)
#ifdef CYGHWR_HAL_STM32_I2C2_REMAP
	pinspec_scl = stm32_i2c_scl( CYGHWR_HAL_STM32_I2C2_SCL );
	pinspec_sda = stm32_i2c_sda( CYGHWR_HAL_STM32_I2C2_SDA );
#else
	pinspec_scl = CYGHWR_HAL_STM32_GPIO(B, 10, ALTFN, 4, OPENDRAIN, PULLUP, 50MHZ);
	pinspec_sda = CYGHWR_HAL_STM32_GPIO(B, 11, ALTFN, 4, OPENDRAIN, PULLUP, 50MHZ);
#endif
#endif

#ifdef CYGINT_DEVS_I2C_CORTEXM_STM32_BUS2_MODE_POLL
        // register functions
        bus->i2c_tx_fn = &cyg_stm32_i2c_tx_poll;
        bus->i2c_rx_fn = &cyg_stm32_i2c_rx_poll;
#endif
#ifdef CYGSEM_DEVS_I2C_CORTEXM_STM32_BUS2_MODE_INT
        // Initialize i2c event interrupt
        cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_I2C2_EV,
                CYGINT_DEVS_I2C_CORTEXM_STM32_BUS2_MODE_INT_EV_PRI,
                (cyg_addrword_t) extra,
                &stm32_i2c_ev_isr,
                &stm32_i2c_ev_dsr,
                &(extra->i2c_ev_interrupt_handle),
                &(extra->i2c_ev_interrupt_data));
        cyg_drv_interrupt_attach(extra->i2c_ev_interrupt_handle);

        // Initialize i2c error interrupt
        cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_I2C2_EE,
                CYGINT_DEVS_I2C_CORTEXM_STM32_BUS2_MODE_INT_EE_PRI,
                (cyg_addrword_t) extra,
                &stm32_i2c_err_isr,
                &stm32_i2c_err_dsr,
                &(extra->i2c_err_interrupt_handle),
                &(extra->i2c_err_interrupt_data));
        cyg_drv_interrupt_attach(extra->i2c_err_interrupt_handle);
        // register functions
        bus->i2c_tx_fn = &cyg_stm32_i2c_tx_int;
        bus->i2c_rx_fn = &cyg_stm32_i2c_rx_int;
#endif
        // Enable peripheral clock
        base = CYGHWR_HAL_STM32_RCC;
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1ENR, rcc);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB1ENR_I2C2);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1ENR, rcc);
#if defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_F1)
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB2ENR, rcc);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB2ENR_IOPB);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB2ENR_AFIO);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB2ENR, rcc);
#elif defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_HIPERFORMANCE)
        // This register exists, but not these bits.
#endif

        // Reset peripheral
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1RSTR, rcc);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB1ENR_I2C2);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1RSTR, rcc);
        rcc &= ~BIT_(CYGHWR_HAL_STM32_RCC_APB1ENR_I2C2);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1RSTR, rcc);
    }
    else if(extra->i2c_base == CYGHWR_HAL_STM32_I2C3)
    {
#ifdef CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS3_BUSFREQ
        bus_freq = CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS3_BUSFREQ;
#endif
#ifdef CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS3_DUTY_CYCLE_16TO9
        duty = 1;
#endif
#ifdef CYGSEM_DEVS_I2C_CORTEXM_STM32_BUS3_MODE
        smbus = 1;
#endif
#ifdef CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS3_ADDRESS
        bus_addr = CYGNUM_DEVS_I2C_CORTEXM_STM32_BUS3_ADDRESS;
#endif
#ifdef CYGHWR_HAL_STM32_I2C3_REMAP
    pinspec_scl = stm32_i2c_scl( CYGHWR_HAL_STM32_I2C3_SCL );
    pinspec_sda = stm32_i2c_sda( CYGHWR_HAL_STM32_I2C3_SDA );
#else
    pinspec_scl = CYGHWR_HAL_STM32_GPIO(H, 7, ALTFN, 4, OPENDRAIN, PULLUP, 50MHZ);
    pinspec_sda = CYGHWR_HAL_STM32_GPIO(H, 8, ALTFN, 4, OPENDRAIN, PULLUP, 50MHZ);
#endif

#ifdef CYGINT_DEVS_I2C_CORTEXM_STM32_BUS3_MODE_POLL
        // register functions
        bus->i2c_tx_fn = &cyg_stm32_i2c_tx_poll;
        bus->i2c_rx_fn = &cyg_stm32_i2c_rx_poll;
#endif
#ifdef CYGSEM_DEVS_I2C_CORTEXM_STM32_BUS3_MODE_INT
        // Initialize i2c event interrupt
        cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_I2C3_EV,
                CYGINT_DEVS_I2C_CORTEXM_STM32_BUS3_MODE_INT_EV_PRI,
                (cyg_addrword_t) extra,
                &stm32_i2c_ev_isr,
                &stm32_i2c_ev_dsr,
                &(extra->i2c_ev_interrupt_handle),
                &(extra->i2c_ev_interrupt_data));
        cyg_drv_interrupt_attach(extra->i2c_ev_interrupt_handle);

        // Initialize i2c error interrupt
        cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_I2C3_ER,
                CYGINT_DEVS_I2C_CORTEXM_STM32_BUS3_MODE_INT_EE_PRI,
                (cyg_addrword_t) extra,
                &stm32_i2c_err_isr,
                &stm32_i2c_err_dsr,
                &(extra->i2c_err_interrupt_handle),
                &(extra->i2c_err_interrupt_data));
        cyg_drv_interrupt_attach(extra->i2c_err_interrupt_handle);
        // register functions
        bus->i2c_tx_fn = &cyg_stm32_i2c_tx_int;
        bus->i2c_rx_fn = &cyg_stm32_i2c_rx_int;
#endif
        // Enable peripheral clock
        base = CYGHWR_HAL_STM32_RCC;
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1ENR, rcc);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB1ENR_I2C3);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1ENR, rcc);
#if defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_F1)
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB2ENR, rcc);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB2ENR_IOPB);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB2ENR_AFIO);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB2ENR, rcc);
#elif defined (CYGHWR_HAL_CORTEXM_STM32_FAMILY_HIPERFORMANCE)
        // This register exists, but not these bits.
#endif

        // Reset peripheral
        HAL_READ_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1RSTR, rcc);
        rcc |= BIT_(CYGHWR_HAL_STM32_RCC_APB1ENR_I2C3);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1RSTR, rcc);
        rcc &= ~BIT_(CYGHWR_HAL_STM32_RCC_APB1ENR_I2C3);
        HAL_WRITE_UINT32 (base + CYGHWR_HAL_STM32_RCC_APB1RSTR, rcc);
    }

    CYGHWR_HAL_STM32_GPIO_SET(pinspec_scl);
    CYGHWR_HAL_STM32_GPIO_SET(pinspec_sda);

    // Software reset to be double sure things are in a default state
    base = extra->i2c_base + CYGHWR_HAL_STM32_I2C_CR1;
    HAL_READ_UINT16(base, reg);
    reg |= CYGHWR_HAL_STM32_I2C_CR1_SWRST;
    HAL_WRITE_UINT16(base, reg);
    reg &= ~CYGHWR_HAL_STM32_I2C_CR1_SWRST;
    HAL_WRITE_UINT16(base, reg);

    // Setup I2C peripheral clock and disable interrupts as a side effect
    // Interrupts will be managed during transfer.
    base = extra->i2c_base + CYGHWR_HAL_STM32_I2C_CR2;
    reg = CYGHWR_HAL_STM32_I2C_CR2_FREQ(hal_stm32_pclk1/1000000);
    HAL_WRITE_UINT16(base, reg);

    // Disable I2C peripheral because the datasheet says some commands
    // don't work with it enabled, for example slew rate.
    base = extra->i2c_base + CYGHWR_HAL_STM32_I2C_CR1;
    HAL_READ_UINT16(base, reg);
    reg &= ~CYGHWR_HAL_STM32_I2C_CR1_PE; // Disable peripheral
    HAL_WRITE_UINT16(base, reg);

    // Setup I2C bus frequency and set bus mode
    base = extra->i2c_base + CYGHWR_HAL_STM32_I2C_CCR;
    reg = 0;
    if(bus_freq <= 100000)
    {
        // Standard mode
        reg = (hal_stm32_pclk1 / bus_freq) / 2; // CCR = Thigh/Tpclk
        if(reg < 0x04)
            reg = 0x04;
        reg &= CYGHWR_HAL_STM32_I2C_CCR_MASK;
        reg &= ~CYGHWR_HAL_STM32_I2C_CCR_F_S; // Set standard mode
        reg &= ~CYGHWR_HAL_STM32_I2C_CCR_DUTY; // ignored in standard
        HAL_WRITE_UINT16(base, reg);

        // Setup I2C rise time for standard mode
        base = extra->i2c_base + CYGHWR_HAL_STM32_I2C_TRISE;
        tmp = hal_stm32_pclk1/1000000;
        reg = (tmp + 1) & CYGHWR_HAL_STM32_I2C_CCR_MASK;
        HAL_WRITE_UINT16(base, reg);
    }
    else
    {
        // Fast mode
        if(duty == 0)
        {
            // Fast mode speed calculate: Tlow/Thigh = 2
            reg = (hal_stm32_pclk1 / bus_freq) / 3;
            reg &= CYGHWR_HAL_STM32_I2C_CCR_MASK;
            reg &= ~CYGHWR_HAL_STM32_I2C_CCR_DUTY;
        }
        else
        {
            // Fast mode speed calculate: Tlow/Thigh = 16/9
            reg = (hal_stm32_pclk1 / bus_freq) / 25;
            reg &= CYGHWR_HAL_STM32_I2C_CCR_MASK;
            reg |= CYGHWR_HAL_STM32_I2C_CCR_DUTY;
        }
        if((reg & CYGHWR_HAL_STM32_I2C_CCR_MASK) == 0)
            reg |= (cyg_uint16)0x0001;
        reg |= CYGHWR_HAL_STM32_I2C_CCR_F_S; // Set fast mode
        HAL_WRITE_UINT16(base, reg);

        // Set I2C rise time for fast mode
        base = extra->i2c_base + CYGHWR_HAL_STM32_I2C_TRISE;
        tmp = hal_stm32_pclk1/1000000;
        reg = (((tmp * 300) / 1000) + 1) & CYGHWR_HAL_STM32_I2C_CCR_MASK;
        HAL_WRITE_UINT16(base, reg);
    }

    // Set bus for I2C mode without ARP, PEC and SMBUS support
    base = extra->i2c_base + CYGHWR_HAL_STM32_I2C_CR1;
    HAL_READ_UINT16(base, reg);
    if (smbus == 1)
    {
        reg |= CYGHWR_HAL_STM32_I2C_CR1_SMBUS; // Enable SMBus mode
        reg |= CYGHWR_HAL_STM32_I2C_CR1_SMBTYPE; // SMBUS Host
    }
    else
    {
        reg &= ~CYGHWR_HAL_STM32_I2C_CR1_SMBUS; // Enable I2C mode
        reg &= ~CYGHWR_HAL_STM32_I2C_CR1_SMBTYPE; // SMBUS Device (ignored)
    }
    reg &= ~CYGHWR_HAL_STM32_I2C_CR1_ENARP; // Disable ARP
    reg &= ~CYGHWR_HAL_STM32_I2C_CR1_ENPEC; // Disable PEC
    reg |= CYGHWR_HAL_STM32_I2C_CR1_ACK; // Enable ACK
    HAL_WRITE_UINT16(base, reg);

    // Set own address register
    base = extra->i2c_base + CYGHWR_HAL_STM32_I2C_OAR1;
    reg = CYGHWR_HAL_STM32_I2C_OAR1_ADD(bus_addr << 1); // Own Address
    reg &= ~CYGHWR_HAL_STM32_I2C_OAR1_ADDMODE; // 7bit address acked
    HAL_WRITE_UINT16(base, reg);

    // Enable I2C peripheral it is ready to use
    base = extra->i2c_base + CYGHWR_HAL_STM32_I2C_CR1;
    HAL_READ_UINT16(base, reg);
    reg |= CYGHWR_HAL_STM32_I2C_CR1_PE;
    HAL_WRITE_UINT16(base, reg);

    // Initialize synchronization objects
    cyg_drv_mutex_init(&extra->i2c_lock);
    cyg_drv_cond_init(&extra->i2c_wait, &extra->i2c_lock);

    // Initialize state field
    extra->i2c_state = I2C_STATE_IDLE;

#if CYGPKG_DEVS_I2C_CORTEXM_STM32_DEBUG_LEVEL > 0
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_CR1, reg);
    debug_printf("I2C: CR1 = 0x%04x\n", reg);
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_CR2, reg);
    debug_printf("I2C: CR2 = 0x%04x\n", reg);
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_OAR1, reg);
    debug_printf("I2C: OAR1 = 0x%04x\n", reg);
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_CCR, reg);
    debug_printf("I2C: CCR = 0x%04x\n", reg);
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_TRISE, reg);
    debug_printf("I2C: TRISE = 0x%04x\n", reg);
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_SR1, reg);
    debug_printf("I2C: SR1 = 0x%04x\n", reg);
    HAL_READ_UINT16(extra->i2c_base + CYGHWR_HAL_STM32_I2C_SR2, reg);
    debug_printf("I2C: SR2 = 0x%04x\n", reg);

    debug_printf("I2C Bus initialized.\n");
#endif
}
