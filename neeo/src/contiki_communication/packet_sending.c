#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <cyg/io/io.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/serialio.h>
#include <cyg/io/ttyio.h>
#include "contiki_communication/communication.h"
#include "contiki_communication/packet_sending.h"
#include "log_helper.h"
#include "jn5168_flasher.h"
#include "msleep.h"

#if 1

#undef LOG_TRACE
#define LOG_TRACE LOG_IGNORE

#endif

int32_t jn5168_send(uint8_t type, uint8_t message_id, uint8_t * buffer, int32_t length)
{
    if(jn_handle == NULL)
    {
        initializeConnection();
    }
    log_msg(LOG_TRACE, __cfunc__, "About to send %d bytes of data.", length);
    hdr_t hdr;
    hdr.start = HeaderMagic;
    hdr.len = length;
    hdr.message_type = type;
    hdr.message_id = message_id;
    hdr.reserved = 0;
    hdr.crc = crc8(&hdr, sizeof(hdr_t) - sizeof(hdr.crc));
    return send_to_jn5168(&hdr, buffer, length);
}

int32_t jn5168_receive(uint8_t * buffer, int32_t max_length, int32_t * buffer_length, int32_t timeout_in_ms, int32_t* result_type)
{
    if(jn_handle == NULL)
    {
        initializeConnection();
    }
    hdr_t hdr;
    if(receive_from_jn5168(&hdr, buffer, max_length, timeout_in_ms) == -1)
    {
        return -1;
    }
    *buffer_length = hdr.len;
    *result_type = hdr.message_type;
    return (int32_t)hdr.message_id;   

}

static uint8_t crc8(const void *vptr, int len)
{
        const uint8_t *data = (uint8_t*)vptr;
        unsigned crc = 0;
        int i, j;
        for (j = len; j; j--, data++) {
                crc ^= (*data << 8);
                for(i = 8; i; i--) {
                        if (crc & 0x8000)
                                crc ^= (0x1070 << 3);
                        crc <<= 1;
                }
                log_msg(LOG_TRACE, __cfunc__, "crc %x j %d", crc, j);
        }
        return (uint8_t)(crc >> 8);
}

static int read_bytes(uint8_t * data, uint32_t length, int32_t useconds)
{
    if(length == 0)
    {
        //shortcut for 0-len read
        return 0;
    }
    uint32_t chars_left = length;
    uint32_t chars_now;
    int32_t actual_read; //for future unblocking implementation
    do {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = useconds;
        chars_now = /*chars_left > 15 ? 15 :*/ chars_left;
        log_msg(LOG_TRACE, __cfunc__, "%d bytes left to read, will read %d waiting for %d ms.", chars_left, chars_now, tv.tv_usec);
        int rc = select(fd + 1, &fds, NULL, NULL, &tv);
        if(rc <= 0)
        {
        
            log_msg(LOG_TRACE, __cfunc__, "Poll failed with %d.", rc);
            
            return -1;
        }
        else
        {
            log_msg(LOG_TRACE, __cfunc__, "Poll succeeded.");
            actual_read = read(fd, data + (length - chars_left), chars_now);
            
            if(actual_read < 0)
            {
                log_msg(LOG_INFO, __cfunc__, "error reading: %d %s", errno, strerror(errno));
            }
            else
            {
                log_msg(LOG_TRACE, __cfunc__, "read %d bytes [0x%02x]", actual_read, *(data+(length - chars_left)));
                // int i =0;
                // for(i = length - chars_left; i < length - chars_left + actual_read; ++i)
                // {
                    // log_msg(LOG_TRACE, __cfunc__, "read char: %c[0x%02x]", data[i], data[i]);
                // }
                chars_left -= actual_read;
            }
            // msleep(WAIT_BETWEEN_SLEEP);
        }
    }while(actual_read >= 0 && chars_left > 0);
    return actual_read < 0; // everything >= 0 is OK
}

static int32_t receive_from_jn5168(hdr_t * hdr, uint8_t * data, int32_t max_data_length, int32_t timeout_in_ms){
    uint8_t * buffer = (uint8_t *)hdr;
    uint8_t header_crc, data_crc, incoming_crc;
    int found_magic = 0;
    uint32_t sizeToRead = 1;
    int timeout_in_us = timeout_in_ms * 1000;
    
    do
    {
        log_msg(LOG_TRACE, __cfunc__, "Looking for magic");
        if(read_bytes(buffer, sizeToRead, timeout_in_us) != 0)
        {
            return -1;
        }
        if(buffer[0] == HeaderMagic)
        {
            found_magic = 1;
        }
    } while(!found_magic);
    log_msg(LOG_TRACE, __cfunc__, "magic found!");
    sizeToRead = sizeof(hdr_t) - sizeof(hdr->start);
    read_bytes(buffer + sizeof(hdr->start), sizeToRead, timeout_in_us);

    header_crc = crc8(hdr, sizeof(hdr_t) - sizeof(hdr->crc));
    if(hdr->crc == header_crc){
        log_msg(LOG_TRACE, __cfunc__, "received correct header!");
        if(hdr->len > max_data_length)
        {
            log_msg(LOG_ERROR, __cfunc__, "max_data_length too small");
            return -1;
        }

        sizeToRead = hdr->len; //with CRC
        
        log_msg(LOG_TRACE, __cfunc__, "size to read: %d, len: %d", sizeToRead, hdr->len);
        read_bytes(data, sizeToRead, timeout_in_us);

        data_crc = crc8(data, hdr->len);
        read_bytes(&incoming_crc, sizeof(incoming_crc), timeout_in_us);
        if(incoming_crc == data_crc)
        {
            log_msg(LOG_TRACE, __cfunc__, "data crc ok");
            return 0;
        }
        else
        {
            log_msg(LOG_WARNING, __cfunc__, "invalid data crc: %X != %X", incoming_crc, data_crc);
        }
    }
    else
    {
        log_msg(LOG_WARNING, __cfunc__, "invalid crc: %X != %X. magic %X type %X id %X len %X reserved %X",
                hdr->crc, header_crc, hdr->start, hdr->message_type, hdr->message_id, hdr->len, hdr->reserved);
    }
    return -1;
            
}

static int32_t send_to_jn5168(hdr_t * hdr, uint8_t * data, int data_length){
    uint8_t buffer[MaxBufferLength] = {0};
    uint32_t actual_write;
    int i;
    Cyg_ErrNo err = ENOERR;

    memcpy(buffer, hdr, sizeof(hdr_t));
    memcpy(buffer + sizeof(hdr_t), data, data_length);

    buffer[sizeof(hdr_t) + hdr->len] = crc8(data, data_length);

    uint32_t total_length = hdr->len + sizeof(hdr_t) + 1; //+1 for CRC

    uint32_t bytes_left = total_length;
    for(i = 0; i < total_length; ++i)
    {
        log_msg(LOG_TRACE, __cfunc__, "0x%X",  buffer[i]);
    }

    while(bytes_left > 0) 
    {
        log_msg(LOG_TRACE, __cfunc__, "Writing %d bytes.", bytes_left);
        actual_write = write(fd, buffer + (total_length - bytes_left), bytes_left);
        //err = cyg_io_write(jn_handle, buffer, &bytes_left);
        bytes_left -= actual_write;
    }
    fsync(fd);

    log_msg(LOG_TRACE, __cfunc__, "TOTAL LEN %d, LEN: %d, HDR CRC: %d, CRC: %d", total_length, hdr->len, hdr->crc, buffer[hdr->len-1]);
    log_msg(LOG_TRACE, __cfunc__, "Message sent.");
    return err;
}

static void initializeConnection()
{
    cyg_serial_info_t ser_info;
    uint32_t infoLen = sizeof(ser_info);
    Cyg_ErrNo err = ENOERR;
    log_msg(LOG_INFO, __cfunc__, "Initializing JN5168 connection");

    jn_power_on(true);
    jn_init_gpio();
    cyg_thread_delay(10);
    jn_reset();

    err = cyg_io_lookup(SERIAL_INTERFACE, &jn_handle);
    if(err < 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error opening serial " SERIAL_INTERFACE ". Response: %d", err);
    }
    log_msg(LOG_INFO, __cfunc__, "Serial found");

    ser_info.baud = CYGNUM_SERIAL_BAUD_115200;
    ser_info.word_length = CYGNUM_SERIAL_WORD_LENGTH_8;
    ser_info.parity = CYGNUM_SERIAL_PARITY_NONE;
    ser_info.stop = CYGNUM_SERIAL_STOP_1;
    ser_info.flags = 0;

    err = cyg_io_set_config(jn_handle, CYG_IO_SET_CONFIG_SERIAL_INFO, &ser_info, &infoLen);
    if(err < 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error setting configuration for serial " SERIAL_INTERFACE ". Response: %d", err);
    }

    log_msg(LOG_INFO, __cfunc__, "Serial initialized, initializing tty");
    
    cyg_tty_info_t tty_info;
    infoLen = sizeof(tty_info);
    cyg_io_handle_t tty_handle;
    err = cyg_io_lookup(TTY_INTERFACE, &tty_handle);
    if(err < 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error opening TTY " TTY_INTERFACE ". Response: %d", err);
    }
    fd = open(TTY_INTERFACE, O_RDWR);
    if(fd < 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Failed to open TTY device: %d.", fd);
        perror("Failed to open TTY");
    }
    else
    {
        log_msg(LOG_INFO, __cfunc__, "Successfully opened FD: %d.", fd);
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
    }


    err = cyg_io_get_config(tty_handle, CYG_IO_GET_CONFIG_TTY_INFO, &tty_info, &infoLen);
    if(err < 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error getting configuration for tty on " TTY_INTERFACE ". Response: %d", err);
    }
    tty_info.tty_out_flags = 0;
    tty_info.tty_in_flags = CYG_TTY_IN_FLAGS_BINARY;
    err = cyg_io_set_config(tty_handle, CYG_IO_SET_CONFIG_TTY_INFO, &tty_info, &infoLen);
    if(err < 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "Error setting configuration for tty on " TTY_INTERFACE ". Response: %d", err);
    }

}

