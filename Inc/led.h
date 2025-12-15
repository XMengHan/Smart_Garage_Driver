#ifndef __LED_H
#define __LED_H

#include "stm32f4xx_hal.h"

/* LED引脚定义 */
#define LED1_PORT   GPIOF
#define LED1_PIN    GPIO_PIN_10

#define LED2_PORT   GPIOC
#define LED2_PIN    GPIO_PIN_0

#define LED3_PORT   GPIOB
#define LED3_PIN    GPIO_PIN_15

#define LED4_PORT   GPIOH
#define LED4_PIN    GPIO_PIN_15

/* LED编号 */
#define LED_1   0
#define LED_2   1
#define LED_3   2
#define LED_4   3
#define LED_ALL 0xFF

/* 函数声明 */
void LED_Init(void);
void LED_On(uint8_t led_num);
void LED_Off(uint8_t led_num);
void LED_Toggle(uint8_t led_num);
void LED_AllOn(void);
void LED_AllOff(void);
void LED_Flow(uint16_t delay_ms, uint8_t times);
void LED_FlashAll(uint16_t delay_ms, uint8_t times);
void LED_Success(void);
void LED_Error(void);
void LED_Heartbeat(void);

#endif /* __LED_H */
