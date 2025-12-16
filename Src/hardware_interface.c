/**
  ******************************************************************************
  * @file    hardware_interface.c
  * @brief   硬件抽象层实现
  * @note    所有底层硬件调用集中在此文件
  ******************************************************************************
  */

#include "hardware_interface.h"
#include "gpio.h"
#include "tim.h"
#include "i2c.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "segment.h"
#include "beep.h"
#include "servo.h"
#include "light_ctrl.h"
#include "led.h"
#include "password.h"
#include "system_data.h"
#include "stdio.h"

/* 外部变量 */
extern __IO uint16_t adcx[4];

/**
  * @brief  硬件统一初始化
  * @note   所有外设和模块的初始化都在这里完成
  */
void HW_Init_All(void)
{
    /* 1. 底层外设初始化 */
    MX_GPIO_Init();
    MX_DMA_Init();           // DMA必须先初始化
    MX_ADC3_Init();
    MX_TIM12_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();

    /* 2. 启动ADC DMA传输 */
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)adcx, 4);

    /* 3. 功能模块初始化 */
    LED_Init();
    Segment_Init(&hi2c1);
    Beep_Init();
    Servo_Init();
    LightCtrl_Init();
    Password_Init();
    
    /* 4. 启动提示 */
    Beep_Tick();
    LED_Flow(100, 2);
    
    printf("\n\r========================================\n\r");
    printf("    智能私家车库系统 v2.0\n\r");
    printf("    [方案A - 完全模块化架构]\n\r");
    printf("========================================\n\r");
    printf("[HW] 硬件初始化完成\n\r");
    /* 5. 启动看门狗（必须放在最后！） */
    MX_IWDG_Init();  // ← 添加这一行
    printf("[IWDG] 看门狗已启动 (超时约5秒)\n\r");
}

/* ==================== ADC接口实现 ==================== */

void HW_ADC_Read(uint16_t *buffer)
{
    for(int i = 0; i < 4; i++)
    {
        buffer[i] = adcx[i];
    }
    SystemData_UpdateADC((uint16_t*)adcx);  // 同步更新到数据层
}

/* ==================== 继电器接口实现 ==================== */

void HW_Relay_On(void)
{
    Relay_On();
    SystemData_SetRelayState(1);
}

void HW_Relay_Off(void)
{
    Relay_Off();
    SystemData_SetRelayState(0);
}

void HW_Relay_Toggle(void)
{
    if(SystemData_GetRelayState())
        HW_Relay_Off();
    else
        HW_Relay_On();
}

/* ==================== LED接口实现 ==================== */

void HW_LED_On(uint8_t led_num)
{
    LED_On(led_num);
}

void HW_LED_Off(uint8_t led_num)
{
    LED_Off(led_num);
}

void HW_LED_AllOn(void)
{
    LED_AllOn();
}

void HW_LED_AllOff(void)
{
    LED_AllOff();
}

void HW_LED_Success(void)
{
    LED_Success();
}

void HW_LED_Error(void)
{
    LED_Error();
}

void HW_LED_FlashAll(uint16_t delay_ms, uint8_t times)
{
    LED_FlashAll(delay_ms, times);
}

/* ==================== 舵机接口实现 ==================== */

void HW_Servo_Open(void)
{
    Servo_Open();
}

void HW_Servo_Close(void)
{
    Servo_Close();
}

void HW_Servo_SetAngle(uint8_t angle)
{
    Servo_SetAngle(angle);
}

/* ==================== 数码管接口实现 ==================== */

void HW_Segment_ShowOPEN(void)
{
    Segment_DisplayOPEN();
}

void HW_Segment_ShowError(void)
{
    Segment_DisplayError();
}

void HW_Segment_ShowTimeout(void)
{
    Segment_DisplayTimeout();
}

void HW_Segment_ShowPASS(void)
{
    Segment_DisplayPASS();
}

void HW_Segment_ShowDash(uint8_t pos)
{
    Segment_DisplayDash(pos);
}

/* ==================== 蜂鸣器接口实现 ==================== */

void HW_Beep_Tick(void)
{
    Beep_Tick();
}

void HW_Beep_OK(void)
{
    Beep_OK();
}

void HW_Beep_Alarm(void)
{
    Beep_Alarm();
}
