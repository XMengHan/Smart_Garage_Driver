/**
  ******************************************************************************
  * @file    non_blocking.h
  * @brief   非阻塞延时工具
  ******************************************************************************
  */

#ifndef __NON_BLOCKING_H
#define __NON_BLOCKING_H

#include "stm32f4xx_hal.h"

/* 非阻塞延时结构 */
typedef struct {
    uint32_t start_tick;
    uint32_t delay_ms;
    uint8_t is_active;
} NonBlock_Delay_t;

/* 函数声明 */
void NonBlock_Delay_Start(NonBlock_Delay_t *nb, uint32_t ms);
uint8_t NonBlock_Delay_Check(NonBlock_Delay_t *nb);
void NonBlock_Delay_Reset(NonBlock_Delay_t *nb);
uint32_t NonBlock_Delay_Elapsed(NonBlock_Delay_t *nb);

#endif /* __NON_BLOCKING_H */
