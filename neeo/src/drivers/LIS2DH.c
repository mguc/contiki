#include <cyg/io/i2c.h>
#include <cyg/io/i2c_stm32.h>
#include <cyg/hal/hal_intr.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdint.h>
#include "log_helper.h"
#include "msleep.h"
#include "LIS2DH.h"

#define LIS2DH_ADDR             0x18

static cyg_i2c_device LIS2DH_i2c_device;

static uint8_t LIS2DH_read_reg(uint8_t reg);
static void LIS2DH_write_reg(uint8_t addr, uint8_t data);

uint8_t LIS2DH_read_reg(uint8_t reg)
{
    uint8_t input = 0xff;

    cyg_i2c_transaction_begin(&LIS2DH_i2c_device);
    if(!cyg_i2c_transaction_tx(&LIS2DH_i2c_device, true, &reg, 1, false)) {
        log_msg(LOG_ERROR, __cfunc__, "TX failed!");
    } else if(!cyg_i2c_transaction_rx(&LIS2DH_i2c_device, true, &input, 1, true, true)) {
        log_msg(LOG_ERROR, __cfunc__, "RX failed!");
    }
    cyg_i2c_transaction_end(&LIS2DH_i2c_device);

    return input;
}

void LIS2DH_write_reg(uint8_t addr, uint8_t data)
{
    uint8_t buffer[2];
    buffer[0] = addr;
    buffer[1] = data;

    cyg_i2c_transaction_begin(&LIS2DH_i2c_device);
    if(!cyg_i2c_transaction_tx(&LIS2DH_i2c_device, true, buffer, 2, true)) {
        log_msg(LOG_ERROR, __cfunc__, "TX failed!");
    }   
    cyg_i2c_transaction_end(&LIS2DH_i2c_device);
}

bool initialized = false;

bool LIS2DH_init(void)
{
    LIS2DH_i2c_device.i2c_bus        = &i2c_bus3;
    LIS2DH_i2c_device.i2c_address    = LIS2DH_ADDR;
    LIS2DH_i2c_device.i2c_flags      = 0;
    LIS2DH_i2c_device.i2c_delay      = 100000;

    if(LIS2DH_read_reg(LIS2DH_WHO_AM_I) != 0x33){
        return false;
    }

    LIS2DH_write_reg(LIS2DH_CTRL_REG1, 0x47);   //50Hz, XYZ Axis Enabled
    LIS2DH_write_reg(LIS2DH_CTRL_REG2, 0x01);   //High Pass filter
    LIS2DH_write_reg(LIS2DH_CTRL_REG3, 0x40);   //Irq
    LIS2DH_write_reg(LIS2DH_CTRL_REG4, 0x88);   //Block data update / Operation mode
    LIS2DH_write_reg(LIS2DH_CTRL_REG5, 0x40);   //Fifo enable
    LIS2DH_write_reg(LIS2DH_CTRL_REG6, 0x00);   //Special irq

    LIS2DH_write_reg(LIS2DH_INT1_CFG, 0x7f);    //Int1 source - XYZ
    LIS2DH_write_reg(LIS2DH_INT1_THS, 0x1f);    //Threshold
    LIS2DH_write_reg(LIS2DH_INT1_DURATION, 0x00); //Irq duration

    return true;
}

int16_t LIS2DH_Read_X() {
    return initialized?((uint16_t)(LIS2DH_read_reg(LIS2DH_OUT_X_H)) << 8) | LIS2DH_read_reg(LIS2DH_OUT_X_L):0;
}

int16_t LIS2DH_Read_Y() {
    return initialized?((uint16_t)(LIS2DH_read_reg(LIS2DH_OUT_Y_H)) << 8) | LIS2DH_read_reg(LIS2DH_OUT_Y_L):0;
}

int16_t LIS2DH_Read_Z() {
    return initialized?((uint16_t)(LIS2DH_read_reg(LIS2DH_OUT_Z_H)) << 8) | LIS2DH_read_reg(LIS2DH_OUT_Z_L):0;
}
