/**
  ******************************************************************************
  * @file    stm324x9i_eval_lcd.c
  * @author  MCD Application Team
  * @version V2.0.2
  * @date    19-June-2014
  * @brief   This file includes the driver for Liquid Crystal Display (LCD) module
  *          mounted on STM324x9I-EVAL evaluation board.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"
#include "stm324x9i_eval_lcd.h"
#include <stdlib.h>
#include <math.h>

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/var_io.h>
#include <cyg/hal/hal_diag.h>

/*******************************************************************************
                       LTDC and DMA2D BSP Routines
*******************************************************************************/

/**
  * @brief  Initializes the LTDC MSP.
  * @param  None
  * @retval None
  */
void LTDC_MUX_Init(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
  if (LCD_PIXCLK <= 432) {
        PeriphClkInitStruct.PLLSAI.PLLSAIN = LCD_PIXCLK;
        PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  } else if (LCD_PIXCLK <= 540) {
        PeriphClkInitStruct.PLLSAI.PLLSAIN = LCD_PIXCLK * 0.8;
        PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
  } else if (LCD_PIXCLK <= 720) {
        PeriphClkInitStruct.PLLSAI.PLLSAIN = LCD_PIXCLK * 0.6;
        PeriphClkInitStruct.PLLSAI.PLLSAIR = 3;
  } else {
        // TODO: >72 MHz is not supported.
        return;
  }
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
 
  /* Enable the LTDC and DMA2D clocks */
  __LTDC_CLK_ENABLE();
  __DMA2D_CLK_ENABLE(); 
  
  /* Enable GPIOs clock */
  #ifdef BOARD_TR2
  __GPIOA_CLK_ENABLE();
  __GPIOG_CLK_ENABLE();
  #endif
  __GPIOI_CLK_ENABLE(); 
  __GPIOJ_CLK_ENABLE();
  __GPIOK_CLK_ENABLE();  

  GPIO_InitTypeDef GPIO_Init_Structure;

  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
  GPIO_Init_Structure.Pull      = GPIO_NOPULL;
  GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;
  GPIO_Init_Structure.Speed     = GPIO_SPEED_HIGH;

  #ifdef BOARD_EVAL
  /* GPIOI configuration */
  GPIO_Init_Structure.Pin       = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; 
  HAL_GPIO_Init(GPIOI, &GPIO_Init_Structure);

  /* GPIOJ configuration */  
  GPIO_Init_Structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                                  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | \
                                  GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
                                  GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; 
  HAL_GPIO_Init(GPIOJ, &GPIO_Init_Structure);  

  /* GPIOK configuration */  
  GPIO_Init_Structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                                  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7; 
  HAL_GPIO_Init(GPIOK, &GPIO_Init_Structure);
  #endif

  #ifdef BOARD_TR2
  GPIO_Init_Structure.Pin = GPIO_PIN_12 | GPIO_PIN_11;
  HAL_GPIO_Init(GPIOA, &GPIO_Init_Structure);

  GPIO_Init_Structure.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOG, &GPIO_Init_Structure);

  GPIO_Init_Structure.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;
  HAL_GPIO_Init(GPIOI, &GPIO_Init_Structure);

  GPIO_Init_Structure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_5 | \
                          GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOJ, &GPIO_Init_Structure);

  GPIO_Init_Structure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | \
                          GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  HAL_GPIO_Init(GPIOK, &GPIO_Init_Structure);

  GPIO_Init_Structure.Pin = GPIO_PIN_10 | GPIO_PIN_12;
  GPIO_Init_Structure.Alternate = GPIO_AF9_LTDC; // TODO: what is AF9 what is AF14?
  HAL_GPIO_Init(GPIOG, &GPIO_Init_Structure);
  #endif
}






// END











#if 0




/**
  * @brief  Initializes the LCD.
  * @param  None
  * @retval LCD state
  */
uint8_t BSP_LCD_Init(void)
{   
#ifdef DEBUG_LCD 
    printf("%s\n\r", __func__);
#endif

    /* Timing Configuration */    
    hltdc_eval.Init.HorizontalSync = (LCD_HSYNC - 1);
    hltdc_eval.Init.VerticalSync = (LCD_VSYNC - 1);
    hltdc_eval.Init.AccumulatedHBP = (LCD_HSYNC + LCD_HBP - 1);
    hltdc_eval.Init.AccumulatedVBP = (LCD_VSYNC + LCD_VBP - 1);
    hltdc_eval.Init.AccumulatedActiveH = (LCD_HEIGHT + LCD_VSYNC + LCD_VBP - 1);
    hltdc_eval.Init.AccumulatedActiveW = (LCD_WIDTH + LCD_HSYNC + LCD_HBP - 1);
    hltdc_eval.Init.TotalHeigh = (LCD_HEIGHT + LCD_VSYNC + LCD_VBP + LCD_VFP - 1);
    hltdc_eval.Init.TotalWidth = (LCD_WIDTH + LCD_HSYNC + LCD_HBP + LCD_HFP - 1);

#ifdef DEBUG_LCD
    printf("HorizontalSync %d \n\r", hltdc_eval.Init.HorizontalSync);
    printf("VerticalSync %d \n\r", hltdc_eval.Init.VerticalSync);
    printf("AccumulatedHBP %d \n\r", hltdc_eval.Init.AccumulatedHBP);
    printf("AccumulatedVBP %d \n\r", hltdc_eval.Init.AccumulatedVBP);
    printf("AccumulatedActiveH %d \n\r", hltdc_eval.Init.AccumulatedActiveH);
    printf("AccumulatedActiveW %d \n\r", hltdc_eval.Init.AccumulatedActiveW);
    printf("TotalHeigh %d \n\r", hltdc_eval.Init.TotalHeigh);
    printf("TotalWidth %d \n\r", hltdc_eval.Init.TotalWidth);
#endif
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
	PeriphClkInitStruct.PLLSAI.PLLSAIN = LCD_PLLSAIN;
	PeriphClkInitStruct.PLLSAI.PLLSAIR = LCD_PLLSAIR;
	PeriphClkInitStruct.PLLSAIDivR = LCD_PLLSAIDivR;
	/*int ret =*/ HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

	/* Initialize the LCD pixel width and pixel height */
	hltdc_eval.LayerCfg->ImageWidth  = LCD_WIDTH;
	hltdc_eval.LayerCfg->ImageHeight = LCD_HEIGHT;

	/* Background value */
	hltdc_eval.Init.Backcolor.Blue = 255;
	hltdc_eval.Init.Backcolor.Green = 255;
	hltdc_eval.Init.Backcolor.Red = 255;

	/* Polarity */
    hltdc_eval.Init.HSPolarity = LCD_HSPolarity;
    hltdc_eval.Init.VSPolarity = LCD_VSPolarity;
    hltdc_eval.Init.DEPolarity = LCD_DEPolarity;
    hltdc_eval.Init.PCPolarity = LCD_PCPolarity;
	hltdc_eval.Instance = LTDC;

	LTDC_MUX_Init();

//	HAL_LTDC_Init(&hltdc_eval);

#ifdef DEBUG_LCD
	printf("BSP_end\r\n");
#endif

	return LCD_OK;
}

void LCD_EnableLineEvent(uint32_t line) {
	HAL_LTDC_ProgramLineEvent(&hltdc_eval, line);
}

void BSP_LCD_ClearInterruptFlags(uint32_t flags) {
	__HAL_LTDC_CLEAR_FLAG(&hltdc_eval,flags);
}

void BSP_LCD_EnableInterrupts(uint32_t interrupts) {
__HAL_LTDC_ENABLE_IT(&hltdc_eval, interrupts);
}

void BSP_LCD_SetBackgroundColor(uint32_t color)
{
       uint32_t tcolor = color & 0x00FFFFFF;

       hltdc_eval.Init.Backcolor.Blue = tcolor & 0x000000ff;
       hltdc_eval.Init.Backcolor.Green = (tcolor & 0x0000ff00) >> 8;
       hltdc_eval.Init.Backcolor.Red = (tcolor & 0x00ff0000) >> 16;

       HAL_LTDC_ChangeBackColor(&hltdc_eval);
}

void EnableDithering(){
	HAL_LTDC_EnableDither(&hltdc_eval);
}

void DisableDithering(){
	HAL_LTDC_DisableDither(&hltdc_eval);
}

/**
  * @brief  Configures the transparency.
  * @param  LayerIndex: Layer foreground or background.
  * @param  Transparency: Transparency
  *           This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF 
  * @retval None
  */

void BSP_LCD_SetTransparency(uint32_t LayerIndex, uint8_t Transparency)
{    
  HAL_LTDC_SetAlphaOnly(&hltdc_eval, Transparency, LayerIndex);
}
/**
  * @brief  Sets display window.
  * @param  LayerIndex: Layer index
  * @param  Xpos: LCD X position
  * @param  Ypos: LCD Y position
  * @param  Width: LCD window width
  * @param  Height: LCD window height  
  * @retval None
  */
void BSP_LCD_SetLayerWindow(uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  /* Reconfigure the layer size */
  HAL_LTDC_SetWindowSize(&hltdc_eval, Width, Height, LayerIndex);
  
  /* Reconfigure the layer position */
  HAL_LTDC_SetWindowPosition(&hltdc_eval, Xpos, Ypos, LayerIndex);
}//Not supported

/**
  * @brief  Configures and sets the color keying.
  * @param  LayerIndex: Layer foreground or background
  * @param  RGBValue: Color reference
  * @retval None
  */
void BSP_LCD_SetColorKeying(uint32_t LayerIndex, uint32_t RGBValue)
{  
  /* Configure and Enable the color Keying for LCD Layer */
  HAL_LTDC_ConfigColorKeying(&hltdc_eval, RGBValue, LayerIndex);
  HAL_LTDC_EnableColorKeying(&hltdc_eval, LayerIndex);
}

/**
  * @brief  Disables the color keying.
  * @param  LayerIndex: Layer foreground or background
  * @retval None
  */
void BSP_LCD_ResetColorKeying(uint32_t LayerIndex)
{
  /* Disable the color Keying for LCD Layer */
  HAL_LTDC_DisableColorKeying(&hltdc_eval, LayerIndex);
}

/**
  * @brief  Enables the display.
  * @param  None
  * @retval None
  */
void BSP_LCD_DisplayOn(void)
{
  /* Display On */
  __HAL_LTDC_ENABLE(&hltdc_eval);
}

/**
  * @brief  Disables the display.
  * @param  None
  * @retval None
  */
void BSP_LCD_DisplayOff(void)
{
  /* Display Off */
  __HAL_LTDC_DISABLE(&hltdc_eval);
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
