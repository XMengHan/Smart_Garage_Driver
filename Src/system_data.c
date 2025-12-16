/**
  ******************************************************************************
  * @file    system_data.c
  * @brief   系统数据单元实现
  ******************************************************************************
  */

#include "system_data.h"
#include "stdio.h"
#include "string.h"

/* 冷热启动魔术数字定义 */
#define BOOT_FLAG_ADDRESS   0x2001FFF0
#define BOOT_MAGIC_NUMBER   0xA5A5A5A5
#define COLD_BOOT_DELAY_MS  500

/* 全局数据实例 */
System_Data_t g_system;

/**
  * @brief  系统数据初始化
  * @note   包含冷热启动检测
  */
void SystemData_Init(void)
{
    uint32_t *boot_flag = (uint32_t *)BOOT_FLAG_ADDRESS;
    
    /* 检查冷热启动 */
    if(*boot_flag == BOOT_MAGIC_NUMBER)
    {
        g_system.cold_boot_flag = 0;  // 热启动
        printf("[DATA] 热启动检测\n\r");
    }
    else
    {
        g_system.cold_boot_flag = 1;  // 冷启动
        *boot_flag = BOOT_MAGIC_NUMBER;
        printf("[DATA] 冷启动检测，延时等待硬件稳定...\n\r");
        HAL_Delay(COLD_BOOT_DELAY_MS);
    }
    
    /* 清空所有数据 */
    memset(&g_system, 0, sizeof(System_Data_t));
    g_system.cold_boot_flag = (*boot_flag == BOOT_MAGIC_NUMBER) ? 0 : 1;
    
    printf("[DATA] 系统数据初始化完成\n\r");
}

/**
  * @brief  重置系统数据（保留统计信息）
  */
void SystemData_Reset(void)
{
    uint32_t success = g_system.success_count;
    uint32_t failed = g_system.failed_count;
    uint32_t timeout = g_system.timeout_count;
    
    memset(&g_system, 0, sizeof(System_Data_t));
    
    g_system.success_count = success;
    g_system.failed_count = failed;
    g_system.timeout_count = timeout;
}

/**
  * @brief  打印系统状态
  */
void SystemData_PrintStatus(void)
{
    printf("\n\r=== 系统状态 ===\n\r");
    printf("成功次数: %lu\n\r", g_system.success_count);
    printf("失败次数: %lu\n\r", g_system.failed_count);
    printf("超时次数: %lu\n\r", g_system.timeout_count);
    printf("继电器: %s\n\r", g_system.relay_state ? "开" : "关");
    printf("光敏电压: %.2fV\n\r", g_system.light_voltage);
    printf("===============\n\r");
}

/**
  * @brief  更新ADC数据
  */
void SystemData_UpdateADC(uint16_t *adc_raw)
{
    for(int i = 0; i < 4; i++)
    {
        g_system.adc_buffer[i] = adc_raw[i];
    }
    g_system.light_voltage = (float)adc_raw[0] * (3.3f / 4096.0f);
}

/**
  * @brief  获取光敏电阻电压
  */
float SystemData_GetLightVoltage(void)
{
    return g_system.light_voltage;
}

/**
  * @brief  添加密码字符
  */
void SystemData_AddPasswordChar(uint8_t digit)
{
    if(g_system.input_index < 6)
    {
        g_system.input_password[g_system.input_index++] = digit;
        g_system.last_input_tick = HAL_GetTick();
    }
}

/**
  * @brief  删除密码字符
  */
void SystemData_DeletePasswordChar(void)
{
    if(g_system.input_index > 0)
    {
        g_system.input_index--;
        g_system.last_input_tick = HAL_GetTick();
    }
}

/**
  * @brief  清空密码
  */
void SystemData_ClearPassword(void)
{
    memset(g_system.input_password, 0, 6);
    g_system.input_index = 0;
}

/**
  * @brief  获取密码长度
  */
uint8_t SystemData_GetPasswordLength(void)
{
    return g_system.input_index;
}

/**
  * @brief  设置继电器状态
  */
void SystemData_SetRelayState(uint8_t state)
{
    g_system.relay_state = state;
}

/**
  * @brief  获取继电器状态
  */
uint8_t SystemData_GetRelayState(void)
{
    return g_system.relay_state;
}

/**
  * @brief  增加成功计数
  */
void SystemData_IncSuccessCount(void)
{
    g_system.success_count++;
}

/**
  * @brief  增加失败计数
  */
void SystemData_IncFailedCount(void)
{
    g_system.failed_count++;
}

/**
  * @brief  增加超时计数
  */
void SystemData_IncTimeoutCount(void)
{
    g_system.timeout_count++;
}
