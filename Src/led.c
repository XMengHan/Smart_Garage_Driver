#include "led.h"
#include "stdio.h"

/* LED状态记录 */
static uint8_t led_states[4] = {0};

/*******************************************************************************
 * @brief  初始化LED
 ******************************************************************************/
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    /* 使能时钟 */
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    
    /* 配置LED1 - PF10 */
    GPIO_InitStruct.Pin = LED1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(LED1_PORT, &GPIO_InitStruct);
    
    /* 配置LED2 - PC0 */
    GPIO_InitStruct.Pin = LED2_PIN;
    HAL_GPIO_Init(LED2_PORT, &GPIO_InitStruct);
    
    /* 配置LED3 - PB15 */
    GPIO_InitStruct.Pin = LED3_PIN;
    HAL_GPIO_Init(LED3_PORT, &GPIO_InitStruct);
    
    /* 配置LED4 - PH15 */
    GPIO_InitStruct.Pin = LED4_PIN;
    HAL_GPIO_Init(LED4_PORT, &GPIO_InitStruct);
    
    /* 默认全部关闭 */
    LED_AllOff();
    
    printf("[LED] 跑马灯初始化完成\n\r");
}

/*******************************************************************************
 * @brief  点亮指定LED
 * @param  led_num: LED编号 (0-3)
 ******************************************************************************/
void LED_On(uint8_t led_num)
{
    if(led_num > 3) return;
    
    switch(led_num)
    {
        case LED_1:
            HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
            led_states[0] = 1;
            break;
        case LED_2:
            HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_RESET);
            led_states[1] = 1;
            break;
        case LED_3:
            HAL_GPIO_WritePin(LED3_PORT, LED3_PIN, GPIO_PIN_RESET);
            led_states[2] = 1;
            break;
        case LED_4:
            HAL_GPIO_WritePin(LED4_PORT, LED4_PIN, GPIO_PIN_RESET);
            led_states[3] = 1;
            break;
    }
}

/*******************************************************************************
 * @brief  熄灭指定LED
 * @param  led_num: LED编号 (0-3)
 ******************************************************************************/
void LED_Off(uint8_t led_num)
{
    if(led_num > 3) return;
    
    switch(led_num)
    {
        case LED_1:
            HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
            led_states[0] = 0;
            break;
        case LED_2:
            HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_SET);
            led_states[1] = 0;
            break;
        case LED_3:
            HAL_GPIO_WritePin(LED3_PORT, LED3_PIN, GPIO_PIN_SET);
            led_states[2] = 0;
            break;
        case LED_4:
            HAL_GPIO_WritePin(LED4_PORT, LED4_PIN, GPIO_PIN_SET);
            led_states[3] = 0;
            break;
    }
}

/*******************************************************************************
 * @brief  翻转指定LED
 ******************************************************************************/
void LED_Toggle(uint8_t led_num)
{
    if(led_num > 3) return;
    
    if(led_states[led_num])
        LED_Off(led_num);
    else
        LED_On(led_num);
}

/*******************************************************************************
 * @brief  点亮所有LED
 ******************************************************************************/
void LED_AllOn(void)
{
    LED_On(LED_1);
    LED_On(LED_2);
    LED_On(LED_3);
    LED_On(LED_4);
}

/*******************************************************************************
 * @brief  熄灭所有LED
 ******************************************************************************/
void LED_AllOff(void)
{
    LED_Off(LED_1);
    LED_Off(LED_2);
    LED_Off(LED_3);
    LED_Off(LED_4);
}

/*******************************************************************************
 * @brief  流水灯效果
 * @param  delay_ms: 每个LED延时（毫秒）
 * @param  times: 循环次数
 ******************************************************************************/
void LED_Flow(uint16_t delay_ms, uint8_t times)
{
    for(uint8_t t = 0; t < times; t++)
    {
        LED_AllOff();
        for(uint8_t i = 0; i < 4; i++)
        {
            LED_On(i);
            HAL_Delay(delay_ms);
            LED_Off(i);
        }
    }
}

/*******************************************************************************
 * @brief  全体闪烁
 * @param  delay_ms: 闪烁间隔（毫秒）
 * @param  times: 闪烁次数
 ******************************************************************************/
void LED_FlashAll(uint16_t delay_ms, uint8_t times)
{
    for(uint8_t i = 0; i < times; i++)
    {
        LED_AllOn();
        HAL_Delay(delay_ms);
        LED_AllOff();
        HAL_Delay(delay_ms);
    }
}

/*******************************************************************************
 * @brief  成功指示灯效果
 ******************************************************************************/
void LED_Success(void)
{
    /* 快速流水2次 */
    LED_Flow(100, 2);
}

/*******************************************************************************
 * @brief  错误指示灯效果
 ******************************************************************************/
void LED_Error(void)
{
    /* 全体快速闪烁3次 */
    LED_FlashAll(150, 3);
}

/*******************************************************************************
 * @brief  心跳指示（在主循环中周期性调用）
 * @note   用于指示系统正常运行
 ******************************************************************************/
void LED_Heartbeat(void)
{
    static uint32_t last_tick = 0;
    static uint8_t led_index = 0;
    
    /* 每500ms移动一次 */
    if((HAL_GetTick() - last_tick) > 500)
    {
        last_tick = HAL_GetTick();
        
        LED_AllOff();
        LED_On(led_index);
        led_index = (led_index + 1) % 4;
    }
}
