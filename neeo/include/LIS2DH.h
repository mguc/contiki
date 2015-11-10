#ifndef _LIS2DH_H
#define _LIS2DH_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdbool.h>

//Registers
#define LIS2DH_STATUS_REG_AUX 	0x07
#define LIS2DH_OUT_TEMP_L 		0x0C
#define LIS2DH_OUT_TEMP_H 		0x0D
#define LIS2DH_INT_COUNTER_REG 	0x0E
#define LIS2DH_WHO_AM_I 		0x0F    // Device identification register
#define LIS2DH_TEMP_CFG_REG 	0x1F
#define LIS2DH_CTRL_REG1 		0x20
#define LIS2DH_CTRL_REG2 		0x21
#define LIS2DH_CTRL_REG3 		0x22
#define LIS2DH_CTRL_REG4 		0x23
#define LIS2DH_CTRL_REG5 		0x24
#define LIS2DH_CTRL_REG6 		0x25
#define LIS2DH_REFERENCE 		0x26
#define LIS2DH_STATUS_REG 		0x27
#define LIS2DH_OUT_X_L 			0x28
#define LIS2DH_OUT_X_H 			0x29
#define LIS2DH_OUT_Y_L 			0x2A
#define LIS2DH_OUT_Y_H 			0x2B
#define LIS2DH_OUT_Z_L 			0x2C
#define LIS2DH_OUT_Z_H 			0x2D
#define LIS2DH_FIFO_CTRL_REG 	0x2E
#define LIS2DH_FIFO_SRC_REG 	0x2F
#define LIS2DH_INT1_CFG 		0x30
#define LIS2DH_INT1_SRC 		0x31
#define LIS2DH_INT1_THS 		0x32
#define LIS2DH_INT1_DURATION 	0x33
#define LIS2DH_INT_CFG 			0x34
#define LIS2DH_INT2_SRC 		0x35
#define LIS2DH_INT2_THS 		0x36
#define LIS2DH_INT2_DURATION 	0x37
#define LIS2DH_CLICK_CFG 		0x38
#define LIS2DH_CLICK_SRC 		0x39
#define LIS2DH_CLICK_THS 		0x3A
#define LIS2DH_TIME_LIMIT 		0x3B
#define LIS2DH_TIME_LATENCY 	0x3C
#define LIS2DH_TIME_WINDOW 		0x3D
#define LIS2DH_ACT_THS 			0x3E
#define LIS2DH_ACT_DUR 			0x3F


bool LIS2DH_init(void);
int16_t LIS2DH_Read_X();
int16_t LIS2DH_Read_Y();
int16_t LIS2DH_Read_Z();

#ifdef __cplusplus
}
#endif

#define __cfunc__ (char*)__func__
#endif /* _LIS2DH_H */
