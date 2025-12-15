#include "beep.h"

/*******************************************************************************
 * @brief  初始化蜂鸣器GPIO
 ******************************************************************************/
void Beep_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    /* 使能GPIOG时钟 */
    BEEP_GPIO_CLK_EN();
    
    /* 配置PG6为推挽输出 */
    GPIO_InitStruct.Pin = BEEP_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BEEP_GPIO_PORT, &GPIO_InitStruct);
    
    /* 默认关闭 */
    HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_RESET);
}

/*******************************************************************************
 * @brief  发出指定频率和时长的声音
 * @param  freq: 频率(Hz), 建议200-2000
 * @param  duration_ms: 持续时间(毫秒)
 ******************************************************************************/
void Beep_Tone(uint16_t freq, uint16_t duration_ms)
{
    uint32_t cycles;
    uint32_t half_period_us;
    
    if(freq == 0) return;
    
    half_period_us = 500000 / freq;  // 半周期(微秒)
    cycles = (uint32_t)freq * duration_ms / 1000;
    
    for(uint32_t i = 0; i < cycles; i++)
    {
        HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_SET);
        for(volatile uint32_t j = 0; j < half_period_us * 2; j++);  // 简易延时
        HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_RESET);
        for(volatile uint32_t j = 0; j < half_period_us * 2; j++);
    }
}

/*******************************************************************************
 * @brief  按键提示音 - 短促"滴"一声 (约50ms)
 ******************************************************************************/
void Beep_Tick(void)
{
    Beep_Tone(1000, 50);  // 1kHz, 50ms
}

/*******************************************************************************
 * @brief  成功提示音 - 欢快的两声
 ******************************************************************************/
void Beep_OK(void)
{
    Beep_Tone(1000, 100);  // 滴
    HAL_Delay(50);
    Beep_Tone(1500, 100);  // 哒（更高音）
    HAL_Delay(50);
    Beep_Tone(2000, 150);  // 嘀~~（最高音，稍长）
}

/*******************************************************************************
 * @brief  报警音 - 长鸣2秒
 ******************************************************************************/
void Beep_Alarm(void)
{
    Beep_Tone(800, 500);   // 嘟——
    HAL_Delay(100);
    Beep_Tone(800, 500);   // 嘟——
    HAL_Delay(100);
    Beep_Tone(800, 800);   // 嘟————
}

/*******************************************************************************
 * @brief  打开蜂鸣器（持续响，需手动关闭）
 * @note   无源蜂鸣器无法直接用这个，仅作为接口保留
 ******************************************************************************/
void Beep_On(void)
{
    // 无源蜂鸣器需要方波驱动，这里用简单的方式
    // 实际项目中建议用定时器PWM
    HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_SET);
}

/*******************************************************************************
 * @brief  关闭蜂鸣器
 ******************************************************************************/
void Beep_Off(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_RESET);
}
