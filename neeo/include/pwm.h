/********************************************************************
 *
 * CONFIDENTIAL
 *
 * ---
 *
 * (c) 2014-2015 Antmicro Ltd <antmicro.com>
 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains
 * the property of Antmicro Ltd.
 * The intellectual and technical concepts contained herein
 * are proprietary to Antmicro Ltd and are protected by trade secret
 * or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Antmicro Ltd.
 *
 * ---
 *
 * Created on: 2015-01-05
 *     Author: wstrozynski@antmicro.com
 *
 */

#ifndef NEEO_INC_PWM_H_
#define NEEO_INC_PWM_H_

#ifdef __cplusplus
 extern "C" {
#endif

void PWM_BUZZER_Init(void);
void PWM_LCD_Init(void);

void PWM_BUZZER_Start(void);
void PWM_LCD_Start(void);

void PWM_BUZZER_Stop(void);
void PWM_LCD_Stop(void);

void PWM_BUZZER_SetFrequency(uint16_t hz);
void PWM_LCD_SetFrequency(uint16_t hz);

void PWM_BUZZER_Beep(uint16_t frequency_hz, uint32_t time_ms);


//void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base);
//void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base);
#ifdef __cplusplus
}
#endif

#endif /* NEEO_INC_PWM_H_ */
