/**
  ******************************************************************************
  * @file    iwdg.c
  * @brief   独立看门狗配置实现
  ******************************************************************************
  */

#include "iwdg.h"

IWDG_HandleTypeDef hiwdg;

/**
  * @brief  独立看门狗初始化
  * @note   超时时间 = (4 * 2^PR) * RL / LSI_Freq
  *         LSI = 32kHz
  *         PR = 6 (分频256)
  *         RL = 625
  *         超时 ≈ 5秒
  */
void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;  // 分频256
    hiwdg.Init.Reload = 1250;                     // 重装载值 (约5秒超时)
    
    if(HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        /* 初始化错误 */
        Error_Handler();
    }
}
