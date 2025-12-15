#include "light_ctrl.h"
#include "stdio.h"

/* 外部ADC数据（由DMA自动更新） */
extern __IO uint16_t adcx[4];

/* 继电器状态 */
static uint8_t relay_state = 0;  // 0=关闭, 1=开启

/* 上次检测时间 */
static uint32_t last_check_tick = 0;

/*******************************************************************************
 * @brief  初始化光控模块
 * @note   需要在ADC和DMA初始化之后调用
 ******************************************************************************/
void LightCtrl_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    /* 使能GPIOG时钟 */
    __HAL_RCC_GPIOG_CLK_ENABLE();
    
    /* 配置PG8为推挽输出 */
    GPIO_InitStruct.Pin = RELAY_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(RELAY_GPIO_PORT, &GPIO_InitStruct);
    
    /* 默认关闭 */
    Relay_Off();
    
    printf("[LIGHT] 光控系统初始化完成\n\r");
}

/*******************************************************************************
 * @brief  打开继电器（照明灯）
 ******************************************************************************/
void Relay_On(void)
{
    if(relay_state == 0)
    {
        HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_GPIO_PIN, GPIO_PIN_SET);
        relay_state = 1;
        printf("[LIGHT] 照明灯 开启\n\r");
    }
}

/*******************************************************************************
 * @brief  关闭继电器（照明灯）
 ******************************************************************************/
void Relay_Off(void)
{
    if(relay_state == 1)
    {
        HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_GPIO_PIN, GPIO_PIN_RESET);
        relay_state = 0;
        printf("[LIGHT] 照明灯 关闭\n\r");
    }
}

/*******************************************************************************
 * @brief  获取光敏电阻电压值
 * @retval 电压值（伏特）
 ******************************************************************************/
float LightCtrl_GetVoltage(void)
{
    /* ADC值转电压：V = ADC * (3.3 / 4096) */
    return (float)adcx[0] * (3.3f / 4096.0f);
}

/*******************************************************************************
 * @brief  光控任务（在主循环中持续调用）
 * @note   每500ms检测一次环境亮度
 ******************************************************************************/
void LightCtrl_Task(void)
{
    /* 限制检测频率 */
    if((HAL_GetTick() - last_check_tick) < LIGHT_CHECK_INTERVAL)
        return;
    
    last_check_tick = HAL_GetTick();
    
    /* 读取光敏电阻电压 */
    float voltage = LightCtrl_GetVoltage();
    
    /* 判断是否需要开灯 */
    if(voltage > LIGHT_THRESHOLD_DARK)
    {
        /* 环境暗 → 开灯 */
        Relay_On();
    }
    else
    {
        /* 环境亮 → 关灯 */
        Relay_Off();
    }
    
    /* 可选：打印调试信息（不要频繁打印，影响性能）*/
    // printf("[LIGHT] 电压: %.2fV\n\r", voltage);
}
