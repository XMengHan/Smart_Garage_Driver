#ifndef __PASSWORD_H
#define __PASSWORD_H

#include "stm32f4xx_hal.h"
#include "segment.h"
#include "beep.h"

/* 密码配置 */
#define PASSWORD_LENGTH       6
#define PASSWORD_TIMEOUT_MS   30000   // 30秒超时

/* 安全配置 */
#define MAX_FAILED_ATTEMPTS   3       // 最大连续失败次数
#define LOCKOUT_DURATION_MS   300000  // 锁定时间：5分钟
#define DELAY_AFTER_FAIL_MS   2000    // 失败后强制延时

/* 密码状态 */
typedef enum {
    PWD_STATE_IDLE = 0,
    PWD_STATE_INPUTTING,
    PWD_STATE_SUCCESS,
    PWD_STATE_FAILED,
    PWD_STATE_TIMEOUT,
    PWD_STATE_LOCKED        // ← 新增：锁定状态
} Password_State_t;

/* 安全统计结构 */
typedef struct {
    uint8_t failed_count;           // 连续失败次数
    uint32_t lockout_start_tick;    // 锁定开始时间
    uint8_t is_locked;              // 锁定标志
    uint32_t total_attempts;        // 总尝试次数
    uint32_t total_success;         // 总成功次数
} Password_Security_t;

/* 函数声明 */
void Password_Init(void);
void Password_StartInput(void);
void Password_Process(uint8_t keycode);
void Password_TimeoutCheck(void);
void Password_Reset(void);
Password_State_t Password_GetState(void);

/* 安全函数 */
uint8_t Password_IsLocked(void);
uint32_t Password_GetLockoutRemaining(void);
void Password_GetSecurityStats(Password_Security_t *stats);
void Password_ClearSecurityStats(void);

#endif /* __PASSWORD_H */
