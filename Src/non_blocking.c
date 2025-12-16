/**
  ******************************************************************************
  * @file    non_blocking.c
  * @brief   非阻塞延时工具实现
  ******************************************************************************
  */

#include "non_blocking.h"

/**
  * @brief  启动非阻塞延时
  * @param  nb: 延时结构指针
  * @param  ms: 延时毫秒数
  */
void NonBlock_Delay_Start(NonBlock_Delay_t *nb, uint32_t ms)
{
    nb->start_tick = HAL_GetTick();
    nb->delay_ms = ms;
    nb->is_active = 1;
}

/**
  * @brief  检查延时是否完成
  * @param  nb: 延时结构指针
  * @retval 1=完成, 0=未完成
  */
uint8_t NonBlock_Delay_Check(NonBlock_Delay_t *nb)
{
    if(!nb->is_active) 
        return 1;  // 已完成
    
    if((HAL_GetTick() - nb->start_tick) >= nb->delay_ms)
    {
        nb->is_active = 0;
        return 1;  // 延时结束
    }
    
    return 0;  // 未结束
}

/**
  * @brief  重置延时计时器
  */
void NonBlock_Delay_Reset(NonBlock_Delay_t *nb)
{
    nb->is_active = 0;
    nb->start_tick = 0;
    nb->delay_ms = 0;
}

/**
  * @brief  获取已经过时间
  * @retval 已过毫秒数
  */
uint32_t NonBlock_Delay_Elapsed(NonBlock_Delay_t *nb)
{
    return HAL_GetTick() - nb->start_tick;
}
