#include "cc3100.h"
#include "host_programming_1.0.0.10.0.h"
#include "host_programming_1.0.0.10.0_signed.h"
#include "simplelink.h"
#include "sl_common.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <cyg/io/io.h>
#include <cyg/io/serialio.h>

#define VERSION_OFFSET      	(8)
#define UCF_OFFSET      		(20)
#define CHUNK_LEN			(1024)
#define LEN_128KB			(131072)

#define UART3_TX CYGHWR_HAL_STM32_PIN_OUT(B, 10, PUSHPULL, NONE, HIGH)

#define find_min(a,b) (((a) < (b)) ? (a) : (b))

#if 1
#define debug_printf( __fmt, ... ) printf("(%s:%s:%d): " __fmt, __FILE__, __FUNCTION__, __LINE__,## __VA_ARGS__ )
#else
#define debug_printf(args...)
#endif

static cyg_io_handle_t cc3100_uart_handle;
const static uint8_t FORMAT_SIZE_1MB[24] = {0x00, 0x17, 0x2D, 0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};

static int cc3100_is_formatted();
static int cc3100_format();
static int cc3100_uart_init();
static void cc3100_uart_break();


int cc3100_get_version(CC3100_Version *version)
{
  unsigned char pConfigOpt = SL_DEVICE_GENERAL_VERSION;
  unsigned char pConfigLen = sizeof(SlVersionFull);
  SlVersionFull ver;

  int ret;
  initClk();
  ret = sl_Start(0, 0, 0);
  if (ret < 0){
	  sl_Stop(0xFF);
	  return -1;
  }
  ret = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &pConfigOpt, &pConfigLen, (unsigned char *)(&ver));
  if (ret < 0){
	  sl_Stop(0xFF);
	  return -1;
  }
  debug_printf("sl_DevGet = %d pConfigLen = %u\n\r", ret, pConfigLen);
  debug_printf("CHIP %x\r\n", (unsigned int)ver.ChipFwAndPhyVersion.ChipId);
  version->chip_id = ver.ChipFwAndPhyVersion.ChipId;
  debug_printf("MAC %d.%d.%d.%d\r\n", (int)ver.ChipFwAndPhyVersion.FwVersion[0], (int)ver.ChipFwAndPhyVersion.FwVersion[1], (int)ver.ChipFwAndPhyVersion.FwVersion[2], (int)ver.ChipFwAndPhyVersion.FwVersion[3]);
  version->fw_version[0] =  ver.ChipFwAndPhyVersion.FwVersion[0];
  version->fw_version[1] =  ver.ChipFwAndPhyVersion.FwVersion[1];
  version->fw_version[2] =  ver.ChipFwAndPhyVersion.FwVersion[2];
  version->fw_version[3] =  ver.ChipFwAndPhyVersion.FwVersion[3];
  debug_printf("PHY %d.%d.%d.%d\r\n", (int)ver.ChipFwAndPhyVersion.PhyVersion[0], (int)ver.ChipFwAndPhyVersion.PhyVersion[1], (int)ver.ChipFwAndPhyVersion.PhyVersion[2], (int)ver.ChipFwAndPhyVersion.PhyVersion[3]);
  version->phy_version[0] = ver.ChipFwAndPhyVersion.PhyVersion[0];
  version->phy_version[1] = ver.ChipFwAndPhyVersion.PhyVersion[1];
  version->phy_version[2] = ver.ChipFwAndPhyVersion.PhyVersion[2];
  version->phy_version[3] = ver.ChipFwAndPhyVersion.PhyVersion[3];
  debug_printf("NWP %d.%d.%d.%d\r\n", (int)ver.NwpVersion[0],(int)ver.NwpVersion[1],(int)ver.NwpVersion[2],(int)ver.NwpVersion[3]);
  version->nwp_version[0] = ver.NwpVersion[0];
  version->nwp_version[1] = ver.NwpVersion[1];
  version->nwp_version[2] = ver.NwpVersion[2];
  version->nwp_version[3] = ver.NwpVersion[3];
  debug_printf("ROM %d \r\n", (int)ver.RomVersion);
  version->rom_version = ver.RomVersion;

  debug_printf("HOST %d.%d.%d.%d\r\n", (int)SL_MAJOR_VERSION_NUM, (int)SL_MINOR_VERSION_NUM, (int)SL_VERSION_NUM, (int)SL_SUB_VERSION_NUM);
  version->host_version[0] = SL_MAJOR_VERSION_NUM;
  version->host_version[1] = SL_MINOR_VERSION_NUM;
  version->host_version[2] = SL_VERSION_NUM;
  version->host_version[3] = SL_SUB_VERSION_NUM;

  ret = sl_Stop(0xFF);

  return 0;
}

int cc3100_check_version(){
  CC3100_Version version;
  int ret;
  ret = cc3100_get_version(&version);
  if(ret < 0){
    debug_printf("Could not check version of CC3100!\r\n");
    return ret;
  }
  int i = 0;
  while(i <= 3){
    if((version.fw_version[i] < LATEST_VERSION.fw_version[i]) \
    || (version.phy_version[i] < LATEST_VERSION.phy_version[i]) \
    || (version.nwp_version[i] < LATEST_VERSION.nwp_version[i])) {
      debug_printf("CC3100's firmware is outdated...\r\n");
      return 1;
    }
    ++i;
  }
  debug_printf("CC3100's firmware is up-to-date\r\n");
  return 0;
}

int cc3100_update(){
	debug_printf("Updating CC3100...\r\n");
	_i32         		fileHandle = -1;
	_u32        		Token = 0;
	int          		retVal = 0;
	unsigned int         		remainingLen, movingOffset, chunkLen;

	if(!cc3100_is_formatted())
	{
		debug_printf("Formatting needed.\r\n");
		if(cc3100_format() != 0) {
			debug_printf("Formatting failed.\r\n");
			return -1;
		}
	}

	/* Initializing the CC3100 device */
	sl_Start(0, 0, 0);

	/* create/open the servicepack file for 128KB with rollback, secured and public write */
	debug_printf("Openning ServicePack file");
	retVal = sl_FsOpen((const _u8 *)"/sys/servicepack.ucf",
			FS_MODE_OPEN_CREATE(LEN_128KB, _FS_FILE_OPEN_FLAG_SECURE|_FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE),
			&Token, &fileHandle);
	if(retVal < 0)
	{
		debug_printf("Cannot open ServicePack file. Error code: %d\r\n", retVal);
		sl_Stop(0xFF);
		return -1;
	}

	/* program the servicepack */
	debug_printf("Programming ServicePack file\r\n");

	remainingLen = sizeof(servicePackImage);
	movingOffset = 0;
	chunkLen = find_min(CHUNK_LEN, remainingLen);

	/* Flashing must be done in 1024 bytes chunks */
	do
	{
		retVal = sl_FsWrite(fileHandle, movingOffset, (_u8 *)&servicePackImage[movingOffset], chunkLen);
		if (retVal < 0)
		{
			debug_printf("Cannot program ServicePack file. Error code: %d\r\n", retVal);
			sl_Stop(0xFF);
			return -1;
		}

		remainingLen -= chunkLen;
		movingOffset += chunkLen;
		chunkLen = find_min(CHUNK_LEN, remainingLen);
	}while (chunkLen > 0);

	/* close the servicepack file */
	debug_printf("Closing ServicePack file\r\n");
	retVal = sl_FsClose(fileHandle, 0, (_u8 *)servicePackImageSig, sizeof(servicePackImageSig));

	if (retVal < 0)
	{
		debug_printf("Cannot close ServicePack file. Error code: %d\r\n", retVal);
		sl_Stop(0xFF);
		return -1;
	}

	debug_printf("ServicePack successfuly programmed\r\n");
	/* Stop the CC3100 device */
	sl_Stop(0xFF);

	debug_printf("Updating CC3100 - DONE!\r\n");
	return 0;
}

/**
  * function to check if the the flash memory of the CC3100 is formatted.
  * @return 1 if device is formatted, 0 if not.
  */
static int cc3100_is_formatted(){
    debug_printf("Checking if serial device is formatted...\r\n");
    int retVal = 1;
    SlFsFileInfo_t fsFileInfo;
    sl_Start(0, 0, 0);
	debug_printf("Getting info of ServicePack file\r\n");
	retVal = sl_FsGetInfo((const _u8 *)"/sys/servicepack.ucf", 0, &fsFileInfo);
	if(retVal < 0)
	{
		debug_printf("Cannot get info of ServicePack file. Error code: %d\r\n", retVal);
		if(retVal == -49){ // SL_FS_NO_DEVICE_IS_LOADED (-49)
			debug_printf("Assuming device is not formatted.\r\n");
			retVal = 0;
		}
		else {
			// if file does not exist SL_FS_ERR_FILE_NOT_EXISTS (-11) is returned
			debug_printf("Assuming device is formatted.\r\n");
			retVal = 1;
		}
	}
    sl_Stop(0xFF);
	return retVal;
}

/**
  * function to format the flash memory of the CC3100.
  * @return 0 if formatting was successful, -1 if not.
  */
static int cc3100_format(){
	debug_printf("Formatting serial flash device - START\r\n");
	uint8_t readBuf[2] = {0,0};
	cyg_uint32 length;
	debug_printf("Uart init\r\n");
	cc3100_uart_init();
	debug_printf("Sending break signal\r\n");
	cc3100_uart_break();
	sl_DeviceDisable();
	cyg_thread_delay(100);
    sl_DeviceEnable();
    length = sizeof(readBuf);
    debug_printf("Waiting for ack...\r\n");
    cyg_io_read(cc3100_uart_handle, readBuf, &length);
    if(!(readBuf[0] == 0x00 && readBuf[1] == 0xCC)){
    	debug_printf("Received wrong pattern, %x %x", readBuf[0], readBuf[1]);
    	return -1;
    }

    debug_printf("Uart init\r\n");
    cc3100_uart_init();
    length = sizeof(FORMAT_SIZE_1MB);
    debug_printf("Sending formatting command\r\n");
    cyg_io_write(cc3100_uart_handle,FORMAT_SIZE_1MB, &length);

    readBuf[0] = 0;
    readBuf[1] = 0;
    length = sizeof(readBuf);
    debug_printf("Waiting for ack...\r\n");
    cyg_io_read(cc3100_uart_handle, readBuf, &length);
    if(!(readBuf[0] == 0x00 && readBuf[1] == 0xCC))
    	return -1;
    debug_printf("Formatting serial flash device - DONE\r\n");
    sl_DeviceDisable();
    cyg_thread_delay(100);
    return 0;
}

/**
  * function to init the serial interface connected to CC3100.
  * @return 0 if init was successful, -1 if not.
  */
static int cc3100_uart_init(){
	int err;
	unsigned int len;
	cyg_serial_info_t ser_info;
	cyg_serial_buf_info_t ser_buf_info;
	cyg_uint8 flushBuffer[16];
    err = cyg_io_lookup("/dev/ser2", &cc3100_uart_handle);
    if(err < 0)
    {
        debug_printf("Error opening serial /dev/ser2. Response: %d\n\r", err);
        return -1;
    }

    ser_info.baud = CYGNUM_SERIAL_BAUD_921600; // this config results in a baud rate of 1Mbit
    ser_info.word_length = CYGNUM_SERIAL_WORD_LENGTH_8;
    ser_info.parity = CYGNUM_SERIAL_PARITY_NONE;
    ser_info.stop = CYGNUM_SERIAL_STOP_1;
    ser_info.flags = 0;
    len = sizeof(ser_info);
    err = cyg_io_set_config(cc3100_uart_handle, CYG_IO_SET_CONFIG_SERIAL_INFO, &ser_info, &len);
    if(err < 0)
    {
        debug_printf("Error setting configuration for serial /dev/ser2. Response: %d\n\r", err);
        return -1;
    }
    // this is a workaround to change to the correct baud rate of 921600
    hal_stm32_uart_setbaud(CYGHWR_HAL_STM32_UART3, 921600);
	debug_printf("Flush uart buffers\r\n");
    do
    {
    	cyg_thread_delay(10);
    	len = sizeof(ser_buf_info);
    	cyg_io_get_config(cc3100_uart_handle,CYG_IO_GET_CONFIG_SERIAL_BUFFER_INFO, &ser_buf_info, &len);
    	if(!ser_buf_info.rx_count)
    		break;
    	len = (ser_buf_info.rx_count > 16 ? 16 : ser_buf_info.rx_count);
    	debug_printf("Discarding %u elements\n\r", len);
    	cyg_io_read(cc3100_uart_handle, flushBuffer, &len);
    }
    while(ser_buf_info.rx_count);

	return 0;
}

/**
  * function to set a break signal on the serial interface.
  */
static void cc3100_uart_break(){
    hal_stm32_gpio_set(UART3_TX);
    hal_stm32_gpio_out(UART3_TX, 0);
}
