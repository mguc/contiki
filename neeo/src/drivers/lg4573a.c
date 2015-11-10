#include <cyg/kernel/kapi.h>
#include <cyg/io/spi.h>
#include <cyg/io/spi_stm32.h>
#include <cyg/hal/var_io.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_diag.h>
#include <stdio.h>
#include <stdint.h>
#include "board_config.h"
#include "lg4573a.h"

static uint8_t commandseq[32];

cyg_spi_cortexm_stm32_device_t spi_lcd = {
#ifdef BOARD_TR2
    .spi_device.spi_bus = &cyg_spi_stm32_bus2.spi_bus,
#endif
#ifdef BOARD_EVAL
    .spi_device.spi_bus = &cyg_spi_stm32_bus3.spi_bus,
#endif
    .dev_num = 0,
    .cl_pol = 1,
    .cl_pha = 1,
    .cl_brate = 4000000,
    .cs_up_udly = 0,
    .cs_dw_udly = 0,
    .tr_bt_udly = 0,
    .bus_16bit = false,
};

void lcd_sendcommand(uint8_t cmd, uint8_t* data, int len)
{
	uint8_t commands[5];

	commands[0] = 0x70;
	commands[1] = cmd;
	cyg_spi_transaction_begin(&spi_lcd.spi_device);
	cyg_spi_transaction_transfer(&spi_lcd.spi_device, true, 2, commands, NULL, false);
	cyg_spi_transaction_end(&spi_lcd.spi_device);
	cyg_thread_delay(1);
	if (len>0)
	{
		commands[0] = 0x72;
		cyg_spi_transaction_begin(&spi_lcd.spi_device);
		cyg_spi_transaction_transfer(&spi_lcd.spi_device, true, 1, commands, NULL, false);
		cyg_spi_transaction_transfer(&spi_lcd.spi_device, true, len, data, NULL, false);
		cyg_spi_transaction_end(&spi_lcd.spi_device);
	}
}

void LG4573A_Init()
{
        // TODO: maybe we should do SWRESET ?

	// C0h – Internal Oscillator Setting
	commandseq[0] = 0x01; // 1 --> enable
	commandseq[1] = 0x1c; // frequency, 0x1c -> 2.56 MHz
	lcd_sendcommand(LG4573A_OSCSET, commandseq, 2);

	// 21h – Display Inversion On
        // lcd_sendcommand(LG4573A_INVON, NULL, 0);

	// 20h – Display Inversion Off
	lcd_sendcommand(LG4573A_INVOFF, NULL, 0);

	// 29h – Display On
	lcd_sendcommand(LG4573A_DISPON, NULL, 0);

	// 3Ah – Interface Pixel Format
	#define BIT16 (0x5 << 4)
        #define BIT18 (0x6 << 4)
        #define BIT24 (0x7 << 4)
	commandseq[0] = BIT24; // AndreasK: 0x77
	lcd_sendcommand(LG4573A_COLMOD, commandseq, 1);

	//B1h – RGB Interface Setting
	#define SYNC (1 << 4)
	#define HBP 10
	#define VBP 107
        #define DE_POLARITY_SHIFT 0
        #define VSYNC_POLARITY_SHIFT 1
        #define HSYNC_POLARITY_SHIFT 2
        #define PCLK_POLARITY_SHIFT 3
        #define DE_ENABLED_SHIFT 4 // =SYNC
	commandseq[0] = (0 << DE_ENABLED_SHIFT) | (1 << PCLK_POLARITY_SHIFT) | (1 << HSYNC_POLARITY_SHIFT) | (1 << VSYNC_POLARITY_SHIFT) | (0 << DE_POLARITY_SHIFT); //AndreasK: 0x06
	commandseq[1] = LCD_HBP & 0x7F;//0x10; //AndreasK: 0x1e  this is ignored if not sync
	commandseq[2] = LCD_VBP;//0x10; //AndreasK: 0x0c
	lcd_sendcommand(LG4573A_RGBIF, commandseq, 3);

	//B2h – Panel Characteristics Setting
	commandseq[0] = (0<<5) | (1<<4) | (0<<1) | (0<<0);	// SWAP | SELP | 480 horiz | REV
	commandseq[1] = (800 / 4); // vertical resolution divided by 4
	lcd_sendcommand(LG4573A_PANELSET, commandseq, 2);

	//B3h – Panel Drive Setting, Dot inversion mode (0,...,3)
	commandseq[0] = 0x00; // 0: column inversion
	lcd_sendcommand(LG4573A_PANELDRV, commandseq, 1);

	//B4h – Display Mode Control DITH – Dither block enable
	commandseq[0] = (1 << 2); // 1 --> DITH enable
	lcd_sendcommand(LG4573A_DISPMOD, commandseq, 1);

	//Display Control 1
	commandseq[0] = 0x10; // source output delay (10 pixel clocks)
	commandseq[1] = 0x30; // equalization period (30 pixel clocks)
	commandseq[2] = 0x30; // GND level period (30 poixel clocks)
	commandseq[3] = 0x00; // SHIZ
	commandseq[4] = 0x00; // SLT -> simulaneous drive
	lcd_sendcommand(LG4573A_DISPCTL1, commandseq, 5);

	//B6h – Display Control 2
	commandseq[0] = (0<<2) | (0<<1) | (1<<0);	// ASG | SDM | FHN  Dual scan //K: 0x01
	commandseq[1] = 0x18; // CLW
	commandseq[2] = 0x02; // GTO
	commandseq[3] = 0x40; // GNO
	commandseq[4] = 0x10; // GVST
	commandseq[5] = 0x00; // GPM
	lcd_sendcommand(LG4573A_DISPCTL2, commandseq, 6);

	// C1h – Power Control 1
	// commandseq[0] = 0x00;
	// lcd_sendcommand(0xc1, commandseq, 1);

	// C2h – Power Control 2
	// commandseq[0] = 0x01;
	// lcd_sendcommand(0xc2, commandseq, 1);

	//C3h – Power Control 3
	commandseq[0] = 0x03; // STMODE 3 --> PFM Boosting, Diode inverting
	commandseq[1] = 0x03; // fosc/16 / fpclk/512
	commandseq[2] = 0x03; // fosc/16 / fpclk/512
	commandseq[3] = 0x05; // ...
	commandseq[4] = 0x04;
	lcd_sendcommand(LG4573A_PWRCTL3, commandseq, 5);


	// C4h – Power Control 4
	commandseq[0] = 0x12;
	commandseq[1] = 0x23;
	commandseq[2] = 0x14;
	commandseq[3] = 0x14;
	commandseq[4] = 0x01;
	commandseq[5] = 0x6c;
	lcd_sendcommand(LG4573A_PWRCTL4, commandseq, 6);

	cyg_thread_delay(200);

	// C5h – Power Control 5
	commandseq[0] = 0x6e;
	lcd_sendcommand(LG4573A_PWRCTL5, commandseq, 1);


	// C6h – Power Control 6
	commandseq[0] = 0x24;
	commandseq[1] = 0x50;
	commandseq[2] = 0x00;
	lcd_sendcommand(LG4573A_PWRCTL6, commandseq, 3);


	commandseq[0] = 0 << 3; /// 36h – Set Address Mode 0<<3: RGB. 1<<0 flip vert, 1<<1 flip hor
	lcd_sendcommand(0x36, commandseq, 1);

	// D0h – Positive Gamma Curve for Red
	// D1h - ... etc

	commandseq[0] = 0x00;
	commandseq[1] = 0x44;
	commandseq[2] = 0x44;
	commandseq[3] = 0x16;
	commandseq[4] = 0x15;
	commandseq[5] = 0x03;
	commandseq[6] = 0x61;
	commandseq[7] = 0x16;
	commandseq[8] = 0x03;

	lcd_sendcommand(LG4573A_RGAMMAP, commandseq, 9);
	lcd_sendcommand(LG4573A_RGAMMAN, commandseq, 9);
	lcd_sendcommand(LG4573A_GGAMMAP, commandseq, 9);
	lcd_sendcommand(LG4573A_GGAMMAN, commandseq, 9);
	lcd_sendcommand(LG4573A_BGAMMAP, commandseq, 9);
	lcd_sendcommand(LG4573A_BGAMMAN, commandseq, 9);

	cyg_thread_delay(5);

	//DISPLAY ON
	// 11h – Sleep Out
	lcd_sendcommand(LG4573A_SLPOUT, NULL, 0);

	cyg_thread_delay(5);
}

