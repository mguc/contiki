#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/io/serialio.h>
#include <stdio.h>
#include "board_config.h"
#include "jn5168_flasher.h"

/* Message Type */
#define JN_MSG_FLASH_ERASE      0x07
#define JN_MSG_FLASH_WRITE      0x09
#define JN_MSG_FLASH_READ       0x0B
#define JN_MSG_SECTOR_ERASE     0x0D
#define JN_MSG_SR_WRITE         0x0F
#define JN_MSG_RAM_WRITE        0x1D
#define JN_MSG_RAM_READ         0x1F
#define JN_MSG_RUN              0x21
#define JN_MSG_FLASH_READ_ID    0x25
#define JN_MSG_CHANGE_BAUD      0x27
#define JN_MSG_FLASH_SELECT     0x2C
#define JN_MSG_GET_CHIP_ID      0x32

/* Flash type for JN_MSG_FLASH_SELECT msg */
#define JN_FLASH_SELECT_M25P10      0x00
#define JN_FLASH_SELECT_25VF010     0x01
#define JN_FLASH_SELECT_25F512      0x02
#define JN_FLASH_SELECT_M25P40      0x03
#define JN_FLASH_SELECT_M25P05      0x04
#define JN_FLASH_SELECT_M25P20      0x05
#define JN_FLASH_SELECT_INTERNAL    0x08

/* UART speed */
#define JN_UART_SPEED_38400         26
#define JN_UART_SPEED_115200        9
#define JN_UART_SPEED_500000        2
#define JN_UART_SPEED_1000000       1

/* Response */
#define JN_RESPONSE_OK                  0x00
#define JN_RESPONSE_NOT_SUPPORTED       0xff
#define JN_RESPONSE_WRITE_FAIL          0xfe
#define JN_RESPONSE_INVALID_RESPONSE    0xfd
#define JN_RESPONSE_CRC_ERROR           0xfc
#define JN_RESPONSE_ASSERT_FAIL         0xfb
#define JN_RESPONSE_USER_INTERRUPT      0xfa
#define JN_RESPONSE_READ_FAIL           0xf9
#define JN_RESPONSE_TST_ERROR           0xf8
#define JN_RESPONSE_AUTH_ERROR          0xf7
#define JN_RESPONSE_NO_RESPONSE         0xf6


#define MAX_DATA_LENGTH                 128
#define JN_MAC_REGION                   0x01001580
#define JN_MAC_REGION_LEN               8
#define JN_CUSTOM_MAC_REGION            0x01001570
#define JN_CUSTOM_MAC_REGION_LEN        8

#define SERIAL_INTERFACE        "/dev/ser1"
#define DEBUG_PRINTF
#define DEBUG_TALK  0

#define RESET_IDLE_STATE    1
#define RESET_ACTIVE_STATE  0

#if defined(BOARD_TR2)
#define RF_PROG CYGHWR_HAL_STM32_PIN_OUT(E, 3, PUSHPULL, NONE, FAST)
#define RF_RESET CYGHWR_HAL_STM32_PIN_OUT(G, 13, PUSHPULL, NONE, FAST)
#define RFLS_OE CYGHWR_HAL_STM32_PIN_OUT(E, 2, PUSHPULL, NONE, FAST)
#elif defined(BOARD_EVAL)
#define RF_PROG CYGHWR_HAL_STM32_PIN_OUT(I, 8, PUSHPULL, NONE, FAST)
#define RF_RESET CYGHWR_HAL_STM32_PIN_OUT(F, 7, PUSHPULL, NONE, FAST)
#endif

#ifdef DEBUG_PRINTF
   #define debug_printf( __fmt, ... ) printf("(mmaj): %s[%d]: " __fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__ )
   #define debug_printf_clear( __fmt, ... ) printf(__fmt, ## __VA_ARGS__ )
#else
   #define debug_printf(args...)
   #define debug_printf_clear(args...)
#endif

cyg_io_handle_t uart_handle;

/*
as the AN-1003 says, the maximal number of bytes to program/read is 128 + 4 bytes of address + 1 byte length + 1 byte CRC = 134 bytes
so the Response should be pointer to array of size 134 bytes (if response is required)
TODO: Response can by any length but talk function should check if we can fit the response in passed buffer
*/
void talk(cyg_uint8 MsgType, cyg_bool ExpectedRespone, cyg_uint32 Address, cyg_uint16 Length, cyg_uint8* Data, cyg_uint32 DataLength, cyg_uint8* Response)
{
    cyg_uint8 MsgHeader[7];
    cyg_uint8 MsgCRC = 0;
    cyg_uint32 i, len = 2;
 
    /* compose the header */
    MsgHeader[1] = MsgType;
    /* message contains address and data */
    if((MsgType == JN_MSG_FLASH_WRITE) || (MsgType == JN_MSG_RAM_WRITE))
    {
        memcpy(MsgHeader+2, &Address, 4);
        len += 4;
    }
    /* message contains address and length  */
    else if((MsgType == JN_MSG_FLASH_READ) || (MsgType == JN_MSG_RAM_READ))
    {
        memcpy(MsgHeader+2, &Address, 4);
        len += 4;
        memcpy(MsgHeader+6, &Length, 2);
        len += 2;
    }
    /* other messages can contain 1 byte of data or no data */
    /* nonetheless we add DataLength to header length (it can be 0 it is nor problem) */
    MsgHeader[0] = len + DataLength;
    /* calculate CRC for header */
    for(i=0; i<len; i++)
        MsgCRC ^= MsgHeader[i];
#if DEBUG_TALK
    debug_printf("SEND: ");
    for(i=0; i<len; i++)
        debug_printf_clear("%02x ", MsgHeader[i]);
#endif
    /* send header */
    cyg_io_write(uart_handle, MsgHeader, &len);
     
    /* append data if available and update CRC */
    if(DataLength)
    {
        for(i = 0; i < DataLength; i++)
            MsgCRC ^= Data[i];
        len = DataLength;
#if DEBUG_TALK
        for(i=0; i<DataLength; i++)
            debug_printf_clear("%02x ", Data[i]);
#endif
        /* send data */
        cyg_io_write(uart_handle, Data, &DataLength);
    }
#if DEBUG_TALK
    debug_printf_clear("%02x ", MsgCRC);
    debug_printf_clear("\n\r");
#endif
    /* send CRC */
    len = 1;
    cyg_io_write(uart_handle, &MsgCRC, &len);
 
    /* check if we expect the response */
    if(!ExpectedRespone)
        return;
    /* Read the respone, first byte is the length of the message */
    len = 1;
    cyg_io_read(uart_handle, Response, &len);
    len = Response[0];
    cyg_io_read(uart_handle, Response+1, &len);
 
#if DEBUG_TALK
    debug_printf("RECV: ");
    for(i=0; i<Response[0]+1; i++)
        debug_printf_clear("%02x ", Response[i]);
    debug_printf_clear("\n\r");
#endif
}

int detect_flash(void)
{
    cyg_uint8 buffer[8];

    talk(JN_MSG_FLASH_READ_ID, true, 0, 0, NULL, 0, buffer);
    if(buffer[3]!=0xcc || buffer[4]!=0xee)
    {
        debug_printf("Couldn't detect Jennic internal flash. Found MNFCT=0x%02x DEVID=0x%02x\n\r", buffer[3], buffer[4]);
        return -1;
    }
    
    debug_printf("Waiting for response\n\r");
    // select flash
    buffer[0] = JN_FLASH_SELECT_INTERNAL;
    talk(JN_MSG_FLASH_SELECT, true, 0, 0, buffer, 1, buffer);
    if(buffer[2])
    {
        debug_printf("Couldn't select flash type. Error: 0x%02x\n\r", buffer[2]);
        return -2;
    }

    return 0;
}

int jn_erase_flash(void)
{
    cyg_uint8 buffer[8];

    talk(JN_MSG_FLASH_ERASE, true, 0, 0, NULL, 0, buffer);

    if(buffer[2])
    {
        debug_printf("Flash erase failed! Response: 0x%02x\n\r", buffer[2]);
        return -1;
    }

    return 0;
}

/* TODO: every read operation should pass the buffer langth and 'talk' function should do sanity checks on the buffer length */
int jn_read_flash(cyg_uint32 Address, cyg_uint32 Length, cyg_uint8* Response)
{
    talk(JN_MSG_FLASH_READ, true, Address, Length, NULL, 0, Response);
    
    if(Response[2])
    {
        debug_printf("Flash read failed! Resonse: 0x%02x\n\r", Response[2]);
        return -1;
    }

    return 0;
}

int jn_write_flash(cyg_uint32 Address, cyg_uint32 Length, cyg_uint8* Data)
{
    cyg_uint8 buffer[4];

    talk(JN_MSG_FLASH_WRITE, true, Address, 0, Data, Length, buffer);

    if(buffer[2])
    {
        debug_printf("Flash write failed! Response: 0x%02x\n\r", buffer[2]);
        return -1;
    }

    return 0;
}

int jn_read_ram(cyg_uint32 Address, cyg_uint32 Length, cyg_uint8* Response)
{
    talk(JN_MSG_RAM_READ, true, Address, Length, NULL, 0, Response);

   if(Response[2])
    {
        debug_printf("RAM read failed! Response: 0x%02x\n\r", Response[2]);
        return -1;
    }

    return 0;
}

int jn_read_mac(cyg_uint8* mac)
{
    cyg_uint8 buffer[12];
 
    if(jn_read_ram(JN_MAC_REGION, JN_MAC_REGION_LEN, buffer) < 0)
    {
        debug_printf("MAC read failed! Response: 0x%02x\n\r", buffer[2]);
        return -1;
    }
 
    memcpy(mac, buffer+3, 8);

    return 0;
}

int jn_write_sr(cyg_uint8 status)
{
    cyg_uint8 buffer[8];
    
    talk(JN_MSG_SR_WRITE, true, 0, 0, &status, 1, buffer);

    if(buffer[2])
    {
        debug_printf("Status Register write failed! Response: 0x%02x\n\r", buffer[2]);
        return -1;
    }

    return 0;
}

int jn_change_speed(cyg_uint8 speed)
{
    cyg_uint8 buffer[8];

    talk(JN_MSG_CHANGE_BAUD, true, 0, 0, &speed, 1, buffer);

    if(buffer[2])
    {
        debug_printf("Changing UART baudrate failed! Response: 0x%02x\n\r", buffer[2]);
        return -1;
    }

    return 0;
}

int jn_upload(cyg_uint8* Program, cyg_uint32 Length)
{
    cyg_uint32 Offset = 0, ChunkLength;
    /* commented out as the printf slows down the flashing process */
    /* cyg_uint32 i = 0; */
    cyg_serial_info_t ser_info;
    Cyg_ErrNo err;
    cyg_uint32 len = sizeof(ser_info);

    /* calculate first chunk length, not longer than MAX_DATA_LENGTH*/
    ChunkLength = (Length < MAX_DATA_LENGTH ? Length : MAX_DATA_LENGTH);

    /* omit first 4 bytes of the image, as it contains an application version number
    that can't be stored in the flash memory. More information:
    JN-AN-1003 (v1.9) 19-Dec-2012
    Note in the middle of page 11 */
    Program += 4;
    Length -= 4;

    /* invoke flash erase */
    if(jn_erase_flash() < 0)
        return -1;

    /* write the application to the flash memory in a chunks */
    while(ChunkLength)
    {
        /* commented out as the printf slows down the flashing process */
        /* debug_printf("Writing [%u] to flash @ 0x%08x len=%u\n\r", i++, Offset, ChunkLength);*/
        /* Offset is an address at which the chunk will be stored
        ChunkLength can't be longer than MAX_DATA_LEGTH
        Program + Offset is a pointer to the begin of actual Chunk in our RAM */
        if(jn_write_flash(Offset, ChunkLength, Program + Offset) < 0)
            return -2;
        Offset += ChunkLength;
        /* calculate ChunkLength to be no more than MAX_DATA_LENGTH */
        if(Length - Offset > MAX_DATA_LENGTH)
            ChunkLength = MAX_DATA_LENGTH;
        else
            ChunkLength = Length - Offset;
    }
    
    ser_info.baud = CYGNUM_SERIAL_BAUD_115200;
    ser_info.word_length = CYGNUM_SERIAL_WORD_LENGTH_8;
    ser_info.parity = CYGNUM_SERIAL_PARITY_NONE;
    ser_info.stop = CYGNUM_SERIAL_STOP_1;
    ser_info.flags = 0;

    err = cyg_io_set_config(uart_handle, CYG_IO_SET_CONFIG_SERIAL_INFO, &ser_info, &len);
    if(err < 0)
    {
        debug_printf("Error setting configuration for serial " SERIAL_INTERFACE ". Response: %d\n\r", err);
        return -3;
    }

    /* reset JN516x to start new application */
    cyg_thread_delay(10);
    hal_stm32_gpio_out(RF_RESET, RESET_ACTIVE_STATE);
    cyg_thread_delay(10);
    hal_stm32_gpio_out(RF_RESET, RESET_IDLE_STATE);
    /* turn off level shifter */
    return 0;
}

void jn_reset(void)
{
    /* reset JN516x to start new application */
    cyg_thread_delay(10);
    hal_stm32_gpio_out(RF_RESET, RESET_ACTIVE_STATE);
    cyg_thread_delay(10);
    hal_stm32_gpio_out(RF_RESET, RESET_IDLE_STATE);
}

void jn_power_on(bool state)
{
#ifdef BOARD_TR2
    hal_stm32_gpio_set(RFLS_OE);
    hal_stm32_gpio_out(RFLS_OE, (state?1:0));
#endif
}

void jn_init_gpio(void)
{
    /* Initialize GPIOs required by download process */
    /* TODO: move that to some general init of hardware? */
    hal_stm32_gpio_set(RF_PROG);
    hal_stm32_gpio_set(RF_RESET);
    hal_stm32_gpio_out(RF_PROG, 1);
    hal_stm32_gpio_out(RF_RESET, RESET_IDLE_STATE);    

    /* turn on power for JN5168 */
    jn_power_on(true);
}

int jn_init_bootloader(void)
{
    Cyg_ErrNo err;
    cyg_serial_info_t ser_info;
    cyg_serial_buf_info_t ser_buf_info;
    cyg_uint32 len = sizeof(ser_info);
    cyg_uint8 buffer[16];

    /* initialize GPIOs, multiple call of this function is safe */
    jn_init_gpio();

    /* to start bootloader on JN516x module
    assert RESET and PROG low, then release RESET
    after releasing RESET JN5168 enters bootloader mode
    by detecting PROG is low, then release PROG */
    hal_stm32_gpio_out(RF_PROG, 0);
    hal_stm32_gpio_out(RF_RESET, RESET_ACTIVE_STATE);

    err = cyg_io_lookup(SERIAL_INTERFACE, &uart_handle);
    if(err < 0)
    {
        debug_printf("Error opening serial " SERIAL_INTERFACE ". Response: %d\n\r", err);
        /* make sure to release RESET without starting bootloader
        it prevents leaving JN516x in reset state and allows JN5168 to normally work */
        hal_stm32_gpio_out(RF_PROG, 1);
        cyg_thread_delay(1);
        hal_stm32_gpio_out(RF_RESET, RESET_IDLE_STATE);
        return -1;
    }

    ser_info.baud = CYGNUM_SERIAL_BAUD_38400;
    ser_info.word_length = CYGNUM_SERIAL_WORD_LENGTH_8;
    ser_info.parity = CYGNUM_SERIAL_PARITY_NONE;
    ser_info.stop = CYGNUM_SERIAL_STOP_1;
    ser_info.flags = 0;

    err = cyg_io_set_config(uart_handle, CYG_IO_SET_CONFIG_SERIAL_INFO, &ser_info, &len);
    if(err < 0)
    {
        debug_printf("Error setting configuration for serial " SERIAL_INTERFACE ". Response: %d\n\r", err);
        /* make sure to release RESET without starting bootloader
        it prevents leaving JN516x in reset state and allows JN5168 to normally work */
        hal_stm32_gpio_out(RF_PROG, 1);
        cyg_thread_delay(1);
        hal_stm32_gpio_out(RF_RESET, RESET_IDLE_STATE);
        return -2;
    }

    debug_printf("Entering flashing mode... \n\r");
    hal_stm32_gpio_out(RF_RESET, RESET_IDLE_STATE);
    cyg_thread_delay(10);

    debug_printf("Flushing input buffers...\n\r");
    do
    {
        cyg_thread_delay(10);
        len = sizeof(ser_buf_info);
        cyg_io_get_config(uart_handle,CYG_IO_GET_CONFIG_SERIAL_BUFFER_INFO, &ser_buf_info, &len);
        if(!ser_buf_info.rx_count)
            break;
        len = (ser_buf_info.rx_count > 16 ? 16 : ser_buf_info.rx_count);
        debug_printf("Discarding %u elements\n\r", len);
            cyg_io_read(uart_handle, buffer, &len);
    }
    while(ser_buf_info.rx_count);

    /* deassert PROG for normal operation of JN5168 */
    cyg_thread_delay(100);
    hal_stm32_gpio_out(RF_PROG, 1);

    debug_printf("Detecting and selecting flash...!\n\r");
    if(detect_flash() < 0)
    {
        /* make sure to release RESET without starting bootloader
        it prevents leaving JN516x in reset state and allows JN5168 to normally work */
        hal_stm32_gpio_out(RF_RESET, RESET_ACTIVE_STATE);
        hal_stm32_gpio_out(RF_PROG, 1);
        cyg_thread_delay(1);
        hal_stm32_gpio_out(RF_RESET, RESET_IDLE_STATE);
        return -3;
    }

    debug_printf("Changing JN5168 UART speed...\n\r");
    if(jn_change_speed(JN_UART_SPEED_1000000) < 0)
    {
        /* make sure to release RESET without starting bootloader
        it prevents leaving JN516x in reset state and allows JN5168 to normally work */
        hal_stm32_gpio_out(RF_RESET, RESET_ACTIVE_STATE);
        hal_stm32_gpio_out(RF_PROG, 1);
        cyg_thread_delay(1);
        hal_stm32_gpio_out(RF_RESET, RESET_IDLE_STATE);
        return -4;
    }

    debug_printf("Changing STM32 UART speed...\n\r");
    ser_info.baud = CYGNUM_SERIAL_BAUD_921600; /* in fact it is 1Mbit */
    ser_info.word_length = CYGNUM_SERIAL_WORD_LENGTH_8;
    ser_info.parity = CYGNUM_SERIAL_PARITY_NONE;
    ser_info.stop = CYGNUM_SERIAL_STOP_1;
    ser_info.flags = 0;
    len = sizeof(ser_info);
    err = cyg_io_set_config(uart_handle, CYG_IO_SET_CONFIG_SERIAL_INFO, &ser_info, &len);

    if(err < 0)
    {
        debug_printf("Error setting configuration for serial " SERIAL_INTERFACE ". Response: %d\n\r", err);
        /* make sure to release RESET without starting bootloader
        it prevents leaving JN516x in reset state and allows JN5168 to normally work */
        hal_stm32_gpio_out(RF_RESET, RESET_ACTIVE_STATE);
        hal_stm32_gpio_out(RF_PROG, 1);
        cyg_thread_delay(1);
        hal_stm32_gpio_out(RF_RESET, RESET_IDLE_STATE);
        return -5;
    }

    debug_printf("Init done!\n\r");

    return 0;
}
