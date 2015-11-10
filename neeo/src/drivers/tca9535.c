#include <cyg/io/i2c.h>
#include <cyg/io/i2c_stm32.h>
#include <cyg/hal/hal_intr.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdint.h>
#include "log_helper.h"
#include "msleep.h"
#include "tca9535.h"

#define TCA9535_ADDR                     0x20

/* TCA9535 Registers Map */
#define TCA9535_INPUT_PORT0              0x00
#define TCA9535_INPUT_PORT1              0x01
#define TCA9535_OUTPUT_PORT0             0x02
#define TCA9535_OUTPUT_PORT1             0x03
#define TCA9535_POLARITY_INVERSION_PORT0 0x04
#define TCA9535_POLARITY_INVERSION_PORT1 0x05
#define TCA9535_CONFIGURATION_PORT0      0x06
#define TCA9535_CONFIGURATION_PORT1      0x07

// GPIOs connected to TC9535
#define KP_INT CYGHWR_HAL_STM32_PIN_IN(B, 14, NONE)
#define KP_RST CYGHWR_HAL_STM32_PIN_OUT(B, 11, PUSHPULL, NONE, HIGH)

// useful for log_helper
#define __cfunc__ (char*)__func__

static cyg_i2c_device kp_i2c_device;
static cyg_interrupt kp_int_object;
static cyg_handle_t kp_int_handle;
static sem_t kp_sem;

cyg_uint32 tca9535_intISR(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_interrupt_mask(vector);
    cyg_interrupt_acknowledge(vector);

    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_EXTI+CYGHWR_HAL_STM32_EXTI_PR, CYGHWR_HAL_STM32_EXTI_BIT(14));

    return CYG_ISR_CALL_DSR | CYG_ISR_HANDLED;
}

void tca9535_intDSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    int semValue = 0;

    sem_getvalue(&kp_sem, &semValue);
    if(semValue == 0){
        sem_post(&kp_sem);
    }
    cyg_interrupt_unmask(vector);
}

static uint8_t tca9535_read_reg(uint8_t reg)
{
    uint8_t input = 0xff;

    cyg_i2c_transaction_begin(&kp_i2c_device);
    if(!cyg_i2c_transaction_tx(&kp_i2c_device, true, &reg, 1, false)) {
        log_msg(LOG_ERROR, __cfunc__, "TX failed!");
    } else if(!cyg_i2c_transaction_rx(&kp_i2c_device, true, &input, 1, true, true)) {
        log_msg(LOG_ERROR, __cfunc__, "RX failed!");
    }
    cyg_i2c_transaction_end(&kp_i2c_device);

    return input;
}

static uint16_t tca9535_read_input_internal(void)
{
    uint8_t buffer;
    uint8_t input[2];

    buffer = TCA9535_INPUT_PORT0;
    input[0] = 0;
    input[1] = 0;

    cyg_i2c_transaction_begin(&kp_i2c_device);
    if(!cyg_i2c_transaction_tx(&kp_i2c_device, true, &buffer, 1, false)) {
        log_msg(LOG_ERROR, __cfunc__, "TX failed!");
    } else if(!cyg_i2c_transaction_rx(&kp_i2c_device, true, input, 2, true, true)) {
        log_msg(LOG_ERROR, __cfunc__, "RX failed!");
    }
    cyg_i2c_transaction_end(&kp_i2c_device);

    return (((uint16_t)(input[1])<<8) | input[0]);
}


static void tca9535_write_reg(uint8_t addr, uint8_t data)
{
    uint8_t buffer[2];
    buffer[0] = addr;
    buffer[1] = data;

    cyg_i2c_transaction_begin(&kp_i2c_device);
    if(!cyg_i2c_transaction_tx(&kp_i2c_device, true, buffer, 2, true)) {
        log_msg(LOG_ERROR, __cfunc__, "TX failed!");
    }   
    cyg_i2c_transaction_end(&kp_i2c_device);
}

bool tca9535_init(void)
{
    uint32_t regval;

    kp_i2c_device.i2c_bus        = &i2c_bus3;
    kp_i2c_device.i2c_address    = TCA9535_ADDR;
    kp_i2c_device.i2c_flags      = 0;
    kp_i2c_device.i2c_delay      = 100000;

    // reset TCA9535
    hal_stm32_gpio_set(KP_RST);
    hal_stm32_gpio_out(KP_RST, 0);
    msleep(1);
    hal_stm32_gpio_out(KP_RST, 1);
    msleep(10);

    if(!((tca9535_read_reg(TCA9535_POLARITY_INVERSION_PORT0) == 0x00) &&
       (tca9535_read_reg(TCA9535_CONFIGURATION_PORT0) == 0xFF)))
    {
        return false;
    }

    tca9535_write_reg(TCA9535_POLARITY_INVERSION_PORT0, 0xFF);
    tca9535_write_reg(TCA9535_POLARITY_INVERSION_PORT1, 0xFF);     

    sem_init(&kp_sem, 0, 0);

    hal_stm32_gpio_set(KP_INT);
    cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_EXTI14, 0, 0, tca9535_intISR, tca9535_intDSR, &kp_int_handle, &kp_int_object);
    cyg_interrupt_configure(CYGNUM_HAL_INTERRUPT_EXTI14, false, false);
    cyg_interrupt_attach(kp_int_handle);
    cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EXTI14);
    HAL_READ_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(14), regval);
    regval |= CYGHWR_HAL_STM32_SYSCFG_EXTICRX_PORTB(14);
    HAL_WRITE_UINT32(CYGHWR_HAL_STM32_SYSCFG + CYGHWR_HAL_STM32_SYSCFG_EXTICR_BY_PIN(14), regval);

    return true;
}

bool tca9535_read_input(uint16_t *state)
{
    *state = 0;

    if(sem_wait(&kp_sem) != -1)
    {
        *state = tca9535_read_input_internal();
        return true;
    }

    return false;
}

