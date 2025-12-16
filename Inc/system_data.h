/**
  ******************************************************************************
  * @file    system_data.h
  * @brief   系统数据单元统一管理
  * @note    所有全局变量和数据结构集中定义
  ******************************************************************************
  */

#ifndef __SYSTEM_DATA_H
#define __SYSTEM_DATA_H

#include "stm32f4xx_hal.h"

/* ==================== 系统数据结构定义 ==================== */

typedef struct {
    /* ADC数据 */
    __IO uint16_t adc_buffer[4];        // ADC原始数据
    float light_voltage;                 // 光敏电阻电压值
    
    /* 密码输入数据 */
    uint8_t input_password[6];           // 当前输入的密码
    uint8_t input_index;                 // 输入位置索引
    uint32_t last_input_tick;            // 上次输入时间戳
    
    /* 红外遥控数据 */
    uint8_t ir_key_flag;                 // 按键标志
    uint8_t ir_keycode;                  // 按键码
    
    /* 系统状态数据 */
    uint8_t relay_state;                 // 继电器状态 (0/1)
    uint8_t led_index;                   // 当前LED索引
    uint32_t led_last_tick;              // LED上次更新时间
    uint32_t light_check_tick;           // 光控检测时间
    
    /* 冷热启动标志 */
    uint8_t cold_boot_flag;              // 0=热启动, 1=冷启动
    
    /* 系统统计数据 */
    uint32_t success_count;              // 成功开门次数
    uint32_t failed_count;               // 失败次数
    uint32_t timeout_count;              // 超时次数
    
} System_Data_t;

/* ==================== 全局数据实例 ==================== */
extern System_Data_t g_system;

/* ==================== 数据操作函数 ==================== */
void SystemData_Init(void);
void SystemData_Reset(void);
void SystemData_PrintStatus(void);

/* ADC数据读写 */
void SystemData_UpdateADC(uint16_t *adc_raw);
float SystemData_GetLightVoltage(void);

/* 密码数据操作 */
void SystemData_AddPasswordChar(uint8_t digit);
void SystemData_DeletePasswordChar(void);
void SystemData_ClearPassword(void);
uint8_t SystemData_GetPasswordLength(void);

/* 继电器状态操作 */
void SystemData_SetRelayState(uint8_t state);
uint8_t SystemData_GetRelayState(void);

/* 统计数据操作 */
void SystemData_IncSuccessCount(void);
void SystemData_IncFailedCount(void);
void SystemData_IncTimeoutCount(void);

#endif /* __SYSTEM_DATA_H */
