#ifndef __LIGHT_CTRL_H
#define __LIGHT_CTRL_H

#include "stm32f4xx_hal.h"
#include "adc.h"

/* 继电器控制引脚 */
#define RELAY_GPIO_PORT     GPIOG
#define RELAY_GPIO_PIN      GPIO_PIN_8

/* 亮度阈值 (电压值)
 * 光敏电阻：光强↑ → 电阻↓ → 电压↓
 *          光强↓ → 电阻↑ → 电压↑
 * 建议阈值：2.0V，大于此值认为是黑夜
 */
#define LIGHT_THRESHOLD_DARK   2.0f   // 黑夜阈值（伏特）
#define LIGHT_CHECK_INTERVAL   500    // 检测间隔（毫秒）

/* 函数声明 */
void LightCtrl_Init(void);
void LightCtrl_Task(void);
void Relay_On(void);
void Relay_Off(void);
float LightCtrl_GetVoltage(void);

#endif /* __LIGHT_CTRL_H */
