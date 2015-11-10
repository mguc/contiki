#include <cyg/io/i2c.h>
#include <cyg/io/i2c_stm32.h>
#include <cyg/hal/hal_intr.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdint.h>
#include "tca6424.h"

#define TCA6424_ADDR                     0x22

/* TCA6424 Registers Map */
#define TCA6424_INPUT_PORT0              0x00
#define TCA6424_INPUT_PORT1              0x01
#define TCA6424_INPUT_PORT2              0x02
//#define TCA6424_                       0x03  // Reserved
#define TCA6424_OUTPUT_PORT0             0x04
#define TCA6424_OUTPUT_PORT1             0x05
#define TCA6424_OUTPUT_PORT2             0x06
//#define TCA6424_                       0x07  // Reserved
#define TCA6424_POLARITY_INVERSION_PORT0 0x08
#define TCA6424_POLARITY_INVERSION_PORT1 0x09
#define TCA6424_POLARITY_INVERSION_PORT2 0x0A
//#define TCA6424_                       0x0B  // Reserved
#define TCA6424_CONFIGURATION_PORT0      0x0C
#define TCA6424_CONFIGURATION_PORT1      0x0D
#define TCA6424_CONFIGURATION_PORT2      0x0E
//#define TCA6424_                       0x0F  // Reserved

#define TCA6424_AUTO_INCREMENT           0x80


// GPIOs connected to TCA6424
#define KP_INT CYGHWR_HAL_STM32_PIN_IN(B, 14, NONE)
#define KP_RST CYGHWR_HAL_STM32_PIN_OUT(B, 11, PUSHPULL, NONE, HIGH)

// useful for log_helper
#define __cfunc__ (char*)__func__

static cyg_i2c_device kp_i2c_device;

static uint8_t tca6424_read_reg(uint8_t reg)
{
    uint8_t input = 0xff;

    cyg_i2c_transaction_begin(&kp_i2c_device);
    if(!cyg_i2c_transaction_tx(&kp_i2c_device, true, &reg, 1, false)) {
    } else if(!cyg_i2c_transaction_rx(&kp_i2c_device, true, &input, 1, true, true)) {
    }
    cyg_i2c_transaction_end(&kp_i2c_device);

    return input;
}

static uint32_t tca6424_read_input_internal(void)
{
    uint8_t buffer;
    uint8_t input[3];

    buffer = TCA6424_INPUT_PORT0 | TCA6424_AUTO_INCREMENT;
    input[0] = 0;
    input[1] = 0;
    input[2] = 0;

    cyg_i2c_transaction_begin(&kp_i2c_device);
    if(!cyg_i2c_transaction_tx(&kp_i2c_device, true, &buffer, 1, false)) {
    } else if(!cyg_i2c_transaction_rx(&kp_i2c_device, true, input, 3, true, true)) {
    }
    cyg_i2c_transaction_end(&kp_i2c_device);

    return (((uint32_t)(input[2])<<16) | ((uint16_t)(input[1])<<8) | input[0]) & 0x00FFFFFF;
}


static void tca6424_write_reg(uint8_t addr, uint8_t data)
{
    uint8_t buffer[2];
    buffer[0] = addr;
    buffer[1] = data;

    cyg_i2c_transaction_begin(&kp_i2c_device);
    if(!cyg_i2c_transaction_tx(&kp_i2c_device, true, buffer, 2, true)) {
    }   
    cyg_i2c_transaction_end(&kp_i2c_device);
}

bool tca6424_init(void)
{
    kp_i2c_device.i2c_bus        = &i2c_bus3;
    kp_i2c_device.i2c_address    = TCA6424_ADDR;
    kp_i2c_device.i2c_flags      = 0;
    kp_i2c_device.i2c_delay      = 100000;

    // reset TCA6424
    hal_stm32_gpio_set(KP_RST);
    hal_stm32_gpio_out(KP_RST, 0);
    cyg_thread_delay(1);
    hal_stm32_gpio_out(KP_RST, 1);
    cyg_thread_delay(10);


    if(!((tca6424_read_reg(TCA6424_POLARITY_INVERSION_PORT0) == 0x00) &&
       (tca6424_read_reg(TCA6424_CONFIGURATION_PORT0) == 0xFF)))
    {
        return false;
    }

    tca6424_write_reg(TCA6424_POLARITY_INVERSION_PORT0, 0xFF);
    tca6424_write_reg(TCA6424_POLARITY_INVERSION_PORT1, 0x3F);
    tca6424_write_reg(TCA6424_POLARITY_INVERSION_PORT2, 0x00);

    return true;
}

bool tca6424_deinit(void)
{
    // reset TCA6424
    hal_stm32_gpio_set(KP_RST);
    hal_stm32_gpio_out(KP_RST, 0);

    return true;
}

bool tca6424_read_input(uint16_t *state, uint16_t *stateP2)
{
    uint32_t input;

    *state = 0;
    *stateP2 = 0;    

    input = tca6424_read_input_internal();
    *state = input;
    *stateP2 = (input >> 16) & 0x00FF;
    return true;

}

