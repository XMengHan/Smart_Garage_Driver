#include "servo.h"
#include "stdio.h"

/*******************************************************************************
 * @brief  初始化舵机
 * @note   需要在 MX_TIM12_Init() 之后调用
 ******************************************************************************/
void Servo_Init(void)
{
    /* 启动PWM输出 */
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    
    /* 初始位置：关闭状态 */
    Servo_Close();
    
    printf("[SERVO] 舵机初始化完成\n\r");
}

/*******************************************************************************
 * @brief  设置舵机角度
 * @param  angle: 角度值 (0-180)
 ******************************************************************************/
void Servo_SetAngle(uint8_t angle)
{
    uint16_t pulse;
    
    /* 限制角度范围 */
    if(angle > 180) angle = 180;
    
    /* 角度转换为脉宽
     * 0°   → 600
     * 90°  → 1500
     * 180° → 2400
     * 公式: pulse = 600 + (angle * 10)
     */
    pulse = SERVO_PULSE_0_DEG + (angle * 10);
    
    /* 设置PWM比较值 */
    __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_1, pulse);
    
    printf("[SERVO] 设置角度: %d° (脉宽:%d)\n\r", angle, pulse);
}

/*******************************************************************************
 * @brief  直接设置PWM脉宽
 * @param  pulse: 脉宽值 (600-2400)
 ******************************************************************************/
void Servo_SetPulse(uint16_t pulse)
{
    if(pulse < SERVO_PULSE_0_DEG) pulse = SERVO_PULSE_0_DEG;
    if(pulse > SERVO_PULSE_180_DEG) pulse = SERVO_PULSE_180_DEG;
    
    __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_1, pulse);
}

/*******************************************************************************
 * @brief  打开闸机（90度）
 ******************************************************************************/
void Servo_Open(void)
{
    printf("[SERVO] 开门...\n\r");
    
    /* 平滑移动到90度，增加视觉效果 */
    uint16_t current = __HAL_TIM_GetCompare(&htim12, TIM_CHANNEL_1);
    uint16_t target = SERVO_PULSE_90_DEG;
    
    if(current < target)
    {
        for(uint16_t i = current; i <= target; i += 20)
        {
            __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_1, i);
            HAL_Delay(10);  // 平滑移动
        }
    }
    else
    {
        for(uint16_t i = current; i >= target; i -= 20)
        {
            __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_1, i);
            HAL_Delay(10);
        }
    }
    
    __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_1, target);
    printf("[SERVO] 闸机已抬起\n\r");
}

/*******************************************************************************
 * @brief  关闭闸机（0度）
 ******************************************************************************/
void Servo_Close(void)
{
    printf("[SERVO] 关门...\n\r");
    
    /* 平滑移动到0度 */
    uint16_t current = __HAL_TIM_GetCompare(&htim12, TIM_CHANNEL_1);
    uint16_t target = SERVO_PULSE_0_DEG;
    
    if(current > target)
    {
        for(uint16_t i = current; i >= target; i -= 20)
        {
            __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_1, i);
            HAL_Delay(10);
        }
    }
    
    __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_1, target);
    printf("[SERVO] 闸机已落下\n\r");
}

/*******************************************************************************
 * @brief  开门并保持指定时间后自动关门
 * @param  seconds: 保持时间（秒）
 ******************************************************************************/
void Servo_OpenWithDelay(uint16_t seconds)
{
    Servo_Open();
    printf("[SERVO] 保持开启 %d 秒...\n\r", seconds);
    HAL_Delay(seconds * 1000);
    Servo_Close();
}
