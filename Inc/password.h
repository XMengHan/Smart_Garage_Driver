#ifndef __PASSWORD_H
#define __PASSWORD_H

#include "stm32f4xx_hal.h"

/* 密码相关定义 */
#define PASSWORD_LENGTH     6
#define PASSWORD_TIMEOUT_MS 30000  // 30秒超时

/* 密码状态枚举 */
typedef enum {
    PWD_STATE_IDLE = 0,    // 空闲，等待触发
    PWD_STATE_INPUT,       // 正在输入密码
    PWD_STATE_SUCCESS,     // 验证成功
    PWD_STATE_FAILED,      // 验证失败
    PWD_STATE_TIMEOUT      // 输入超时
} Password_State_t;

/* 红外键码到数字的映射 */
#define IR_KEY_0    0xB8
#define IR_KEY_1    0x08
#define IR_KEY_2    0x88
#define IR_KEY_3    0x48
#define IR_KEY_4    0xC8
#define IR_KEY_5    0x28
#define IR_KEY_6    0xA8
#define IR_KEY_7    0xE8
#define IR_KEY_8    0x18
#define IR_KEY_9    0x98
#define IR_KEY_DEL  0x78   // 删除键

/* 函数声明 */
void Password_Init(void);
void Password_StartInput(void);
void Password_Process(uint8_t ir_keycode);
Password_State_t Password_GetState(void);
uint8_t Password_GetInputCount(void);
void Password_Reset(void);
void Password_TimeoutCheck(void);

/* 将键码转换为数字，无效返回0xFF */
uint8_t IR_KeyCode_To_Number(uint8_t keycode);

#endif /* __PASSWORD_H */
