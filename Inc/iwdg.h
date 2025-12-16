/**
  ******************************************************************************
  * @file    iwdg.h
  * @brief   独立看门狗配置头文件
  ******************************************************************************
  */

#ifndef __IWDG_H
#define __IWDG_H

#include "stm32f4xx_hal.h"

extern IWDG_HandleTypeDef hiwdg;

void MX_IWDG_Init(void);

#endif /* __IWDG_H */
