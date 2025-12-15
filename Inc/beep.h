#ifndef __BEEP_H
#define __BEEP_H

#include "stm32f4xx_hal.h"

/* 蜂鸣器引脚定义 */
#define BEEP_GPIO_PORT      GPIOG
#define BEEP_GPIO_PIN       GPIO_PIN_6
#define BEEP_GPIO_CLK_EN()  __HAL_RCC_GPIOG_CLK_ENABLE()

/* 函数声明 */
void Beep_Init(void);
void Beep_On(void);
void Beep_Off(void);
void Beep_Tick(void);           // 按键提示音（短促滴一声）
void Beep_OK(void);             // 成功提示音
void Beep_Alarm(void);          // 报警音（长鸣）
void Beep_Tone(uint16_t freq, uint16_t duration_ms);  // 自定义音调

#endif /* __BEEP_H */
