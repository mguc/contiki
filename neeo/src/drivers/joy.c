#include <cyg/kernel/kapi.h>
#include <cyg/posix/pthread.h>
#include <cyg/posix/time.h>
#include <cyg/io/i2c_stm32.h>
#include <cyg/io/i2c.h>
#include <stdio.h>
#include "joy.h"

#define STMPE_ADDR                          0x42
#define STMPE_REG_ID_LOW                    0x00
#define STMPE_REG_ID_HIGH                   0x01
#define STMPE_REG_VERSION                   0x02
#define STMPE_REG_SYSTEM_CONTROL            0x03
#define STMPE_REG_INTERRUPT_ENABLE_LOW      0x08
#define STMPE_REG_INTERRUPT_ENABLE_HIGH     0x09
#define STMPE_REG_INTERRUPT_STATUS_LOW      0x0A
#define STMPE_REG_INTERRUPT_STATUS_HIGH     0x0B
#define STMPE_REG_GPIO_MONITOR_LOW          0x10
#define STMPE_REG_GPIO_MONITOR_HIGH         0x11
#define STMPE_REG_GPIO_SET_STATE_LOW        0x12
#define STMPE_REG_GPIO_SET_STATE_HIGH       0x13
#define STMPE_REG_GPIO_SET_DIRECTION_LOW    0x14
#define STMPE_REG_GPIO_SET_DIRECTION_HIGH   0x15
#define STMPE_REG_GPIO_POLARITY_INV_LOW     0x16
#define STMPE_REG_GPIO_POLARITY_INV_HIGH    0x17

#define MILLISECONDS 1000000

cyg_i2c_device device;

cyg_uint8 read_byte(cyg_uint8 addr)
{
    cyg_uint8 buffer[1];
    cyg_uint8 input[1];
    buffer[0] = addr;

    cyg_i2c_transaction_begin(&device);

    if(!cyg_i2c_transaction_tx(&device, true, &buffer[0], 1, false)) {
        printf("Read Byte: fail TX.\r\n");
    } else if(!cyg_i2c_transaction_rx(&device, true, &input[0], 1, true, true)) {
        printf("Read Byte: fail RX.\r\n");
    }
    cyg_i2c_transaction_end(&device);

    return input[0];
}

void write_byte(cyg_uint8 addr, cyg_uint8 data)
{
    cyg_uint8 buffer[2];
    buffer[0] = addr;
    buffer[1] = data;

    cyg_i2c_transaction_begin(&device);
    if(!cyg_i2c_transaction_tx(&device, true, &buffer[0], 2, true)) {
        printf("Write Byte: fail TX.\n");
    }
    cyg_i2c_transaction_end(&device);
}

void joy_init(void)
{
    struct timespec Delay;
    cyg_uint8 id_l = 0, id_h = 0;

    Delay.tv_sec = 0;
    Delay.tv_nsec = 50 * MILLISECONDS;

    device.i2c_bus        = &i2c_bus1;
    device.i2c_address    = STMPE_ADDR;
    device.i2c_flags      = 0;
    device.i2c_delay      = 100000;

    id_l = read_byte(STMPE_REG_ID_LOW);
    id_h = read_byte(STMPE_REG_ID_HIGH);

    if(id_h != 0x16 && id_l != 0x00)
    {
        printf("STMPE1600 not found!\n\r");
        return;
    }

    // sfortware reset of STMPE1600
    write_byte(STMPE_REG_SYSTEM_CONTROL, 0x80);
    nanosleep(&Delay, NULL);
    write_byte(STMPE_REG_SYSTEM_CONTROL, 0x00);

    // invert polarity of lines connected to joy (they are active low)
    write_byte(STMPE_REG_GPIO_POLARITY_INV_LOW, 0x00);
    write_byte(STMPE_REG_GPIO_POLARITY_INV_HIGH, 0x7c);
}


cyg_uint8 joy_query(void)
{
    cyg_uint8 val;

    val = read_byte(STMPE_REG_GPIO_MONITOR_HIGH);
    val >>= 2;
    val &= 0x1f;

    return val;
}
