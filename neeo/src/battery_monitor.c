#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/io/adc.h>
#include <stdint.h>
#include "log_helper.h"
#include "msleep.h"


#define __cfunc__ (char*)__func__

#define VREF            1.80  // reference voltage
#define ADC_RESOLUTION  4096  // resolution of the ADC (12 bits -> 4096)
#define R_DIV1          100   // value of the upper resistor in the voltage divider (in kOhms)
#define R_DIV2          39    // value of the lower resistor (divider output) (in kOhms)

#define ADC_DEVICE  "/dev/adc10"

static cyg_io_handle_t adc_channel;

int bm_init(void)
{
    Cyg_ErrNo res;
    cyg_uint32 cfg_data;
    cyg_uint32 len;
    cyg_adc_sample_t sample;
    
    res = cyg_io_lookup(ADC_DEVICE, &adc_channel);
    if(res != ENOERR) {
        log_msg(LOG_ERROR, __cfunc__, "ADC device lookup failed!");
        return res;
    }

    // Disable channel
    res = cyg_io_set_config(adc_channel, CYG_IO_SET_CONFIG_ADC_DISABLE, 0, 0);
    if (res != ENOERR) {
        log_msg(LOG_ERROR, __cfunc__, "Failed to disable ADC channel");
        return res;
    }

    // Make channel non-blocking
    cfg_data = 0;
    len = sizeof(cfg_data);
    res = cyg_io_set_config(adc_channel, CYG_IO_SET_CONFIG_READ_BLOCKING, &cfg_data, &len);
    if (res != ENOERR) {
        log_msg(LOG_ERROR, __cfunc__, "Failed to make ADC channel non-blocking\r\n");
        return res;
    }

    // Set channel sampling rate
    cfg_data = 100;
    len = sizeof(cfg_data);
    res = cyg_io_set_config(adc_channel, CYG_IO_SET_CONFIG_ADC_RATE, &cfg_data, &len);
    if (res != ENOERR) {
        log_msg(LOG_ERROR, __cfunc__, "Failed to set ADC channel sampling rate\r\n");
        return res;
    }

    // Flush channel
    do {
        len = sizeof(sample);
        res = cyg_io_read(adc_channel, &sample, &len);
    } while (res == ENOERR);

    // Disable channel
    res = cyg_io_set_config(adc_channel, CYG_IO_SET_CONFIG_ADC_DISABLE, 0, 0);
    if (res != ENOERR) {
        log_msg(LOG_ERROR, __cfunc__, "Failed to disable ADC channel");
        return res;
    }

    return ENOERR;
}

double bm_read(void)
{
    Cyg_ErrNo res;
    cyg_uint32 len;
    cyg_adc_sample_t sample;
    uint32_t avg_sample;
    static cyg_adc_sample_t samples[16];
    static int collected = 0;
    double result;
    int i;

    // Enable channel
    res = cyg_io_set_config(adc_channel, CYG_IO_SET_CONFIG_ADC_ENABLE, 0, 0);
    if (res != ENOERR) {
        log_msg(LOG_ERROR, __cfunc__, "Failed to enable ADC channel\r\n");
        return 0;
    }

    // read until first successful sample, but not more than 10 attempts
    i = 10;
    while(i--) {
        len = sizeof(sample);
        res = cyg_io_read(adc_channel, &sample, &len);
        if (res == ENOERR)
            break;
    }

    if(sample == 0)
        return 0;

    // add to collection of samples
    samples[collected] = sample;
    collected++;
    collected &= 0x0f;
    
    avg_sample = 0;
    for(i = 0; i < 16; i++) {
        if(samples[i] == 0)
            return 0;
        avg_sample += samples[i];
    }

    avg_sample = avg_sample / 16;
    // log_msg(LOG_TRACE, __cfunc__, "avg_sample = %u", avg_sample);
    result = avg_sample * VREF;
    // log_msg(LOG_TRACE, __cfunc__, "result1    = %f", result);
    result = result * (R_DIV1 + R_DIV2);
    // log_msg(LOG_TRACE, __cfunc__, "result2    = %f", result);
    result = result / ADC_RESOLUTION;
    // log_msg(LOG_TRACE, __cfunc__, "result3    = %f", result);
    result = result / R_DIV2;
    // log_msg(LOG_TRACE, __cfunc__, "result4    = %f", result);
    
    // Disable channel
    res = cyg_io_set_config(adc_channel, CYG_IO_SET_CONFIG_ADC_DISABLE, 0, 0);
    if (res != ENOERR) {
        log_msg(LOG_ERROR, __cfunc__, "Failed to disable ADC channel\r\n");
    }

    return result;
}
