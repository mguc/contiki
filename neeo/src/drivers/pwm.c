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

#include <stdint.h>
#include "pwm.h"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include <cyg/posix/pthread.h>
#include <cyg/posix/time.h>
#include <unistd.h>

#define HALF_CPU_FREQ 84000000
#define BUZ_TIM_FREQ 21000000

static TIM_HandleTypeDef htim2;
//static TIM_HandleTypeDef htim3;

static uint32_t BUZ_FREQ = 1000;
static uint32_t BUZ_PERIOD = 1000;
static uint32_t BUZ_WIDTH = 50;

//static uint32_t LCD_FREQ = 10000;
//static uint32_t LCD_PERIOD = 0;
//static uint32_t LCD_WIDTH = 70;

TIM_ClockConfigTypeDef sClockSourceConfig_buz;
TIM_OC_InitTypeDef sConfigOC_buz;

void PWM_BUZZER_Init()
{
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = (HALF_CPU_FREQ / BUZ_TIM_FREQ) - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    BUZ_PERIOD = (BUZ_TIM_FREQ / BUZ_FREQ) - 1;
    htim2.Init.Period = BUZ_PERIOD;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.RepetitionCounter = 0;
    HAL_TIM_Base_Init(&htim2);

    sClockSourceConfig_buz.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig_buz);

    HAL_TIM_PWM_Init(&htim2);

    sConfigOC_buz.OCMode = TIM_OCMODE_PWM1;
    sConfigOC_buz.Pulse = (BUZ_WIDTH * BUZ_PERIOD)/100;
    sConfigOC_buz.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC_buz.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC_buz, TIM_CHANNEL_1);

    PWM_BUZZER_Stop();
}

void PWM_LCD_Init()
{
    //FIXME Not valid - switched to TIM1

    /*TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_OC_InitTypeDef sConfigOC;

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 15;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    LCD_PERIOD = ((90000000) / (LCD_FREQ*16) ) - 1;
    htim3.Init.Period = LCD_PERIOD;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim3);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig);

    HAL_TIM_PWM_Init(&htim3);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = (LCD_WIDTH * (LCD_PERIOD - 1)) / 100;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
    PWM_LCD_Stop();*/
}

void PWM_BUZZER_Start(void){
    BUZ_PERIOD = (BUZ_TIM_FREQ / BUZ_FREQ) - 1;
    htim2.Init.Period = BUZ_PERIOD;
    htim2.Init.RepetitionCounter = 0;
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}
void PWM_LCD_Start(void){
   // HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void PWM_BUZZER_SetFrequency(uint16_t hz){
    BUZ_FREQ = hz;
    BUZ_PERIOD = (BUZ_TIM_FREQ / BUZ_FREQ) - 1;
    HAL_TIM_SetPulse(htim2.Instance, (BUZ_WIDTH * (BUZ_PERIOD) -1) / 100);
    HAL_TIM_SetPeriod(htim2.Instance, BUZ_PERIOD);
}

void PWM_LCD_SetFrequency(uint16_t hz){
    /*HAL_TIM_SetPulse(htim2.Instance, 0); //Should be htim3, but if it is htim3 LCD doesn't work. //TODO Why?
    HAL_TIM_SetPeriod(htim2.Instance, 0);
    LCD_FREQ = hz;
    LCD_PERIOD = ((84000000) / (LCD_FREQ*16) ) - 1;
    PWM_LCD_SetPulseWidth(LCD_WIDTH);
    HAL_TIM_SetPeriod(htim3.Instance, LCD_PERIOD);*/
}

void PWM_BUZZER_Stop(void) {
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
}

void PWM_LCD_Stop(void) {
    /*HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    HAL_TIM_SetPulse(htim3.Instance, 0);
    HAL_TIM_SetPeriod(htim3.Instance, 0);*/
}

void PWM_BUZZER_Beep(uint16_t frequency_hz, uint32_t time_ms) {
    PWM_BUZZER_Stop();
    int sec = 0;
    sec = time_ms / 1000;
    time_ms = time_ms - sec * 1000;
    PWM_BUZZER_SetFrequency(frequency_hz);
    PWM_BUZZER_Start();
    struct timespec Delay;
    Delay.tv_sec=sec;
    Delay.tv_nsec = time_ms * 1000000;
    nanosleep(&Delay, NULL);
    PWM_BUZZER_Stop();
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    if(htim_base->Instance==TIM2)
    {
        __TIM2_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    }
    else if(htim_base->Instance==TIM3)
    {
     /*   __TIM3_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);*/
    }
}

