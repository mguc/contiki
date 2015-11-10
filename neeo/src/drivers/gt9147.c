/* Includes ------------------------------------------------------------------*/
#include <cyg/kernel/kapi.h>
#include <stdio.h>
#include <stdint.h>
#include <cyg/infra/diag.h>
#include <cyg/io/i2c_stm32.h>
#include "gt9147.h"
#include "board_config.h"


#define GTP_ADDRESS 0x5d

// untrusted registers
#define GTP_REG_BAK_REF         0x99D0
#define GTP_REG_MAIN_CLK        0x8020
#define GTP_REG_CHIP_TYPE       0x8000
#define GTP_REG_HAVE_KEY        0x804E
#define GTP_REG_MATRIX_DRVNUM   0x8069     
#define GTP_REG_MATRIX_SENNUM   0x806A
#define GTP_READ_COOR_ADDR      0x814E
#define GTP_REG_SLEEP           0x8040
#define GTP_REG_SENSOR_ID       0x814A
#define GTP_REG_DOZ_BUF         0x814B

// trusted registers
#define GTP_REG_STATUS          0x814E
#define GTP_REG_CONFIG_DATA     0x8047
#define GTP_REG_VERSION         0x8140
//FIXME The same value here. We need to trust the value because of lack of the documentation
#define GTP_REG_CMD             0x8140
#define GTP_REG_POINT_0         0x8150

// commands - we may need them in future
#define GTP_CMD_READ_COORDINATE_STATUS                 0x00
#define GTP_CMD_ORIGINAL_D_VALUE1                      0x01
#define GTP_CMD_ORIGINAL_D_VALUE2                      0x02
#define GTP_CMD_DATUM_UPDATE                           0x03
#define GTP_CMD_DATUM_CALIBRATION                      0x04
#define GTP_CMD_CLOSE_TOUCHSCREEN                      0x05
#define GTP_CMD_ENTER_CHARGING_MODE                    0x06
#define GTP_CMD_QUIT_CHARGING_MODE                     0x07
#define GTP_CMD_CHANGE_GESTURES_TO_AROUSE_FIRMWARE     0x08
#define GTP_CMD_ENTER_STLAVE_PROXIMITY_DETECTION_MODE  0x20
#define GTP_CMD_ENTER_MASTER_PROXIMITY_DETECTION_MODE  0x21
#define GTP_CMD_ENTER_DATA_TRANSMISSION_MODE           0x22
#define GTP_CMD_ENTER_HOST_TRANSMISSION_MODE           0x23
#define GTP_CMD_QUIT_SLAVE_DETECTION_MODE              0x28
#define GTP_CMD_QUIT_MASTER_PROXIMITY_DETECTION_MODE   0x29
#define GTP_CMD_QUIT_DATA_TRANSMISSIONMODE             0x3A
#define GTP_CMD_ESD_PROTECT_MECH                       0xAA

cyg_i2c_device device = {
#ifdef BOARD_EVAL
.i2c_bus        = &i2c_bus3,
#endif
#ifdef BOARD_TR2
.i2c_bus        = &i2c_bus1,
#endif
    .i2c_address    = GTP_ADDRESS,
    .i2c_flags      = 0,
    .i2c_delay      = 100000
};

bool write_byte(uint16_t address, uint8_t data)
{
    bool ret = true;
    uint8_t buffer[3];
    buffer[0] = (address >> 8) & 0xFF;
    buffer[1] = address & 0xFF;
    buffer[2] = data;

    cyg_i2c_transaction_begin(&device);
    if(cyg_i2c_transaction_tx(&device, true, &buffer[0], 3, true) != 3)
        ret = false;
    cyg_i2c_transaction_end(&device);

    return ret;
}

uint8_t read_byte(uint16_t address)
{
    uint8_t buffer[2];
    buffer[0] = (address >> 8) & 0xFF;
    buffer[1] = address & 0xFF;

    cyg_i2c_transaction_begin(&device);
    cyg_i2c_transaction_tx(&device, true, buffer, 2, true);
    cyg_i2c_transaction_rx(&device, true, buffer, 1, true, true);
    cyg_i2c_transaction_end(&device);

    return buffer[0];
}

bool write_sequence(uint16_t address, uint8_t* data, uint16_t data_length)
{
    bool ret = true;
    uint8_t buffer[2];
    buffer[0] = (address >> 8) & 0xFF;
    buffer[1] = address & 0xFF;

    cyg_i2c_transaction_begin(&device);
    if(cyg_i2c_transaction_tx(&device, true, buffer, 2, false) != 2)
        ret = false;
    if(cyg_i2c_transaction_tx(&device, true, data, data_length, true) != data_length)
        ret = false;
    cyg_i2c_transaction_end(&device);

    return ret;
}

bool read_sequence(uint16_t address, uint8_t* data, uint16_t data_length)
{
    bool ret = true;
    uint8_t buffer[2];
    buffer[0] = (address >> 8) & 0xFF;
    buffer[1] = address & 0xFF;

    cyg_i2c_transaction_begin(&device);
    if(cyg_i2c_transaction_tx(&device, true, buffer, 2, true) != 2)
        ret = false;
    if(cyg_i2c_transaction_rx(&device, true, data, data_length, true, true) != data_length)
        ret = false;
    cyg_i2c_transaction_end(&device);

    return ret;
}

bool gtp_reset_touch_status(void)
{
    return write_byte(GTP_REG_STATUS, 0x00);
}  

uint8_t gtp_read_touch_status(void)
{
    uint8_t val;
    
    val = read_byte(GTP_REG_STATUS);

    return val;
}

bool gtp_send_command(uint8_t cmd)
{
    return write_byte(GTP_REG_CMD, cmd);
}

bool gtp_read_id(char* buffer)
{
    uint8_t buf[8];
    int i;

    if(!gtp_send_command(GTP_CMD_READ_COORDINATE_STATUS))
        return false;
    gtp_read_touch_status();
    if(!gtp_reset_touch_status())
        return false;
    
    if(!read_sequence(GTP_REG_VERSION, buf, 8))
        return false;
    for(i=0; i<4; i++)
        if((buf[i]<'0') || (buf[i]>'9'))
            buf[i] = '0';

    snprintf(buffer, 12, "%c%c%c%c.%02x%02x%02x", buf[0], buf[1], buf[2], buf[3], buf[5], buf[6], buf[7]);

    return true;
}

// void gtp_read_config(uint8_t* config)
// {
//     uint8_t buffer[2];
 
//     buffer[0] = (GTP_REG_CONFIG_DATA >> 8) & 0x00ff;
//     buffer[1] = GTP_REG_CONFIG_DATA & 0xff;

//     cyg_i2c_transaction_begin(&device);
//     cyg_i2c_transaction_tx(&device, true, &buffer[0], 2, true);
//     cyg_i2c_transaction_end(&device);

//     cyg_i2c_transaction_begin(&device);
//     cyg_i2c_transaction_rx(&device, true, config, 228, true, true);
//     cyg_i2c_transaction_end(&device);

// }

// void gtp_write_config(uint8_t* config)
// {
//     uint8_t buffer[240];
     
//     memcpy(buffer+2, config, 228);
//     buffer[0] = (GTP_REG_CONFIG_DATA >> 8) & 0x00ff;
//     buffer[1] = GTP_REG_CONFIG_DATA & 0xff;
    
//     cyg_i2c_transaction_begin(&device);
//     cyg_i2c_transaction_tx(&device, true, buffer, 228, true);
//     cyg_i2c_transaction_end(&device);
// }

uint32_t gtp_read_touch_point(uint16_t tpnum)
{
    uint8_t buffer[8];
    // Read coordinates of touch screen, these are 4 I2c registers
    // Here we just give back the values in a uint32_t
    uint16_t tpaddr = GTP_REG_POINT_0 + (8 * tpnum);

    read_sequence(tpaddr, buffer, 4);

    return buffer[1] << 24 | buffer[0]  << 16 | buffer[3]  << 8 | buffer[2];  
}  

void gtp_get_points(struct GTP_DATA* touch)
{
    uint32_t status, coordinate;
    uint32_t x,y;

	gtp_send_command(GTP_CMD_READ_COORDINATE_STATUS);
    status = gtp_read_touch_status();
    if(status & 0x80)
    {
        if(status & 0x0f)
        {
            //TODO: support for multitouch?
            coordinate = gtp_read_touch_point(0);
            y = coordinate & 0x0000ffff;
            x = coordinate & 0xffff0000;
            x >>= 16;
            touch->touch_flag = 0x01;
            touch->x = x;
            touch->y = y;
        }
        else
        {
            touch->touch_flag = 0;
        }
        gtp_reset_touch_status();
    }
}
