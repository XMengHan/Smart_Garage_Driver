/**
  ******************************************************************************
  * @file    hardware_interface.h
  * @brief   硬件抽象层 - 统一硬件操作接口
  * @note    所有硬件操作必须通过此接口调用
  ******************************************************************************
  */

#ifndef __HARDWARE_INTERFACE_H
#define __HARDWARE_INTERFACE_H

#include "stm32f4xx_hal.h"

/* ==================== 初始化接口 ==================== */
void HW_Init_All(void);

/* ==================== ADC接口 ==================== */
void HW_ADC_Read(uint16_t *buffer);

/* ==================== 继电器接口 ==================== */
void HW_Relay_On(void);
void HW_Relay_Off(void);
void HW_Relay_Toggle(void);

/* ==================== LED接口 ==================== */
void HW_LED_On(uint8_t led_num);
void HW_LED_Off(uint8_t led_num);
void HW_LED_AllOn(void);
void HW_LED_AllOff(void);
void HW_LED_Success(void);
void HW_LED_Error(void);
void HW_LED_FlashAll(uint16_t delay_ms, uint8_t times);

/* ==================== 舵机接口 ==================== */
void HW_Servo_Open(void);
void HW_Servo_Close(void);
void HW_Servo_SetAngle(uint8_t angle);

/* ==================== 数码管接口 ==================== */
void HW_Segment_ShowOPEN(void);
void HW_Segment_ShowError(void);
void HW_Segment_ShowTimeout(void);
void HW_Segment_ShowPASS(void);
void HW_Segment_ShowDash(uint8_t pos);

/* ==================== 蜂鸣器接口 ==================== */
void HW_Beep_Tick(void);
void HW_Beep_OK(void);
void HW_Beep_Alarm(void);

#endif /* __HARDWARE_INTERFACE_H */
