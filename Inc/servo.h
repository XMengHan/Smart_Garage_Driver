#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f4xx_hal.h"
#include "tim.h"

/* 舵机角度对应的脉宽 */
#define SERVO_PULSE_0_DEG      600    // 0度
#define SERVO_PULSE_90_DEG     1500   // 90度
#define SERVO_PULSE_180_DEG    2400   // 180度

/* 车库闸机角度定义 */
#define SERVO_ANGLE_CLOSED     0      // 关闭状态（栏杆水平）
#define SERVO_ANGLE_OPEN       90     // 开启状态（栏杆竖直）

/* 函数声明 */
void Servo_Init(void);
void Servo_SetAngle(uint8_t angle);
void Servo_SetPulse(uint16_t pulse);
void Servo_Open(void);
void Servo_Close(void);
void Servo_OpenWithDelay(uint16_t seconds);

#endif /* __SERVO_H */
