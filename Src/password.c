/**
  ******************************************************************************
  * @file    password.c
  * @brief   密码管理模块 - 安全加固版
  * @note    包含防暴力破解、时序攻击防护等安全机制
  ******************************************************************************
  */

#include "password.h"
#include "string.h"
#include "stdio.h"

/* 密码存储（加密存储在Flash） */
#define PASSWORD_FLASH_ADDR  0x080E0000  // Flash扇区11起始地址
#define PASSWORD_XOR_KEY     0xA5        // 简单异或加密密钥

/* 正确密码（出厂默认，运行时从Flash读取） */
static const uint8_t default_password[PASSWORD_LENGTH] = {1, 2, 3, 4, 5, 6};
static uint8_t runtime_password[PASSWORD_LENGTH];

/* 输入缓冲区 */
static uint8_t input_buffer[PASSWORD_LENGTH];
static uint8_t input_index = 0;

/* 状态变量 */
static Password_State_t current_state = PWD_STATE_IDLE;
static uint32_t last_input_tick = 0;

/* 安全变量 */
static Password_Security_t security = {0};

/* 私有函数 */
static void Password_LoadFromFlash(void);
static void Password_EncryptDecrypt(uint8_t *data, uint8_t len);
static uint8_t Password_ConstTimeCompare(const uint8_t *a, const uint8_t *b, uint8_t len);
static void Password_EnterLockout(void);
static void Password_AddRandomDelay(void);

/**
  * @brief  密码模块初始化
  */
void Password_Init(void)
{
    /* 从Flash加载加密密码 */
    Password_LoadFromFlash();
    
    /* 清空输入缓冲 */
    memset(input_buffer, 0, PASSWORD_LENGTH);
    input_index = 0;
    
    /* 清空安全统计 */
    memset(&security, 0, sizeof(Password_Security_t));
    
    current_state = PWD_STATE_IDLE;
    
    printf("[PWD] 密码系统初始化完成（安全模式）\n\r");
}

/**
  * @brief  从Flash加载加密密码
  * @note   密码在Flash中异或加密存储
  */
static void Password_LoadFromFlash(void)
{
    uint8_t *flash_ptr = (uint8_t *)PASSWORD_FLASH_ADDR;
    
    /* 检查是否已写入密码 */
    if(flash_ptr[0] == 0xFF)
    {
        /* 首次使用，使用默认密码 */
        memcpy(runtime_password, default_password, PASSWORD_LENGTH);
        printf("[PWD] 使用默认密码\n\r");
    }
    else
    {
        /* 从Flash读取并解密 */
        memcpy(runtime_password, flash_ptr, PASSWORD_LENGTH);
        Password_EncryptDecrypt(runtime_password, PASSWORD_LENGTH);
        printf("[PWD] 从Flash加载密码\n\r");
    }
}

/**
  * @brief  简单异或加密/解密
  * @note   实际产品应使用AES等强加密算法
  */
static void Password_EncryptDecrypt(uint8_t *data, uint8_t len)
{
    for(uint8_t i = 0; i < len; i++)
    {
        data[i] ^= PASSWORD_XOR_KEY;
    }
}

/**
  * @brief  恒定时间比较（防时序攻击）
  * @note   无论密码是否正确，比较时间始终相同
  * @retval 1=相等, 0=不等
  */
static uint8_t Password_ConstTimeCompare(const uint8_t *a, const uint8_t *b, uint8_t len)
{
    uint8_t result = 0;
    
    /* 始终比较所有字节，不提前退出 */
    for(uint8_t i = 0; i < len; i++)
    {
        result |= (a[i] ^ b[i]);
    }
    
    /* 添加随机延时，进一步混淆时序 */
    Password_AddRandomDelay();
    
    return (result == 0) ? 1 : 0;
}

/**
  * @brief  添加随机延时（5-20ms）
  * @note   防止时序分析攻击
  */
static void Password_AddRandomDelay(void)
{
    uint32_t random_delay = 5 + (HAL_GetTick() % 15);  // 5-20ms
    HAL_Delay(random_delay);
}

/**
  * @brief  进入锁定状态
  */
static void Password_EnterLockout(void)
{
    security.is_locked = 1;
    security.lockout_start_tick = HAL_GetTick();
    current_state = PWD_STATE_LOCKED;
    
    printf("[PWD] ⚠️ 系统已锁定 %lu 秒\n\r", LOCKOUT_DURATION_MS / 1000);
    
    /* 锁定提示 */
    Segment_DisplayError();
    Beep_Alarm();
}

/**
  * @brief  开始密码输入
  */
void Password_StartInput(void)
{
    /* 检查是否被锁定 */
    if(security.is_locked)
    {
        uint32_t elapsed = HAL_GetTick() - security.lockout_start_tick;
        if(elapsed < LOCKOUT_DURATION_MS)
        {
            uint32_t remaining = (LOCKOUT_DURATION_MS - elapsed) / 1000;
            printf("[PWD] 系统锁定中，剩余 %lu 秒\n\r", remaining);
            current_state = PWD_STATE_LOCKED;
            return;
        }
        else
        {
            /* 解除锁定 */
            security.is_locked = 0;
            security.failed_count = 0;
            printf("[PWD] 锁定解除\n\r");
        }
    }
    
    memset(input_buffer, 0, PASSWORD_LENGTH);
    input_index = 0;
    current_state = PWD_STATE_INPUTTING;
    last_input_tick = HAL_GetTick();
    
    Segment_DisplayPASS();
    printf("[PWD] 等待输入...\n\r");
}

/**
  * @brief  处理按键输入
  */
void Password_Process(uint8_t keycode)
{
    /* 锁定状态拒绝输入 */
    if(security.is_locked)
    {
        Beep_Alarm();
        return;
    }
    
    if(current_state != PWD_STATE_INPUTTING)
        return;
    
    /* 更新输入时间戳 */
    last_input_tick = HAL_GetTick();
    
    /* 删除键 */
    if(keycode == 0x0E)
    {
        if(input_index > 0)
        {
            input_index--;
            Segment_DisplayDash(input_index);
            Beep_Tick();
        }
        return;
    }
    
    /* 数字键 0-9 */
    if(keycode <= 9)
    {
        if(input_index < PASSWORD_LENGTH)
        {
            input_buffer[input_index++] = keycode;
            Segment_DisplayDash(input_index);
            Beep_Tick();
            
            /* 输入完成 */
            if(input_index == PASSWORD_LENGTH)
            {
                security.total_attempts++;
                
                /* 恒定时间比较密码 */
                if(Password_ConstTimeCompare(input_buffer, runtime_password, PASSWORD_LENGTH))
                {
                    /* 密码正确 */
                    current_state = PWD_STATE_SUCCESS;
                    security.failed_count = 0;  // 清空失败计数
                    security.total_success++;
                    printf("[PWD] ✓ 密码正确\n\r");
                }
                else
                {
                    /* 密码错误 */
                    security.failed_count++;
                    current_state = PWD_STATE_FAILED;
                    
                    printf("[PWD] ✗ 密码错误 (%d/%d)\n\r", 
                           security.failed_count, MAX_FAILED_ATTEMPTS);
                    
                    /* 检查是否达到锁定条件 */
                    if(security.failed_count >= MAX_FAILED_ATTEMPTS)
                    {
                        Password_EnterLockout();
                    }
                    else
                    {
                        /* 失败后强制延时 */
                        HAL_Delay(DELAY_AFTER_FAIL_MS);
                    }
                }
            }
        }
    }
}

/**
  * @brief  超时检查
  */
void Password_TimeoutCheck(void)
{
    if(current_state == PWD_STATE_INPUTTING)
    {
        if((HAL_GetTick() - last_input_tick) > PASSWORD_TIMEOUT_MS)
        {
            current_state = PWD_STATE_TIMEOUT;
            printf("[PWD] ⏱ 输入超时\n\r");
        }
    }
}

/**
  * @brief  重置密码模块
  */
void Password_Reset(void)
{
    memset(input_buffer, 0, PASSWORD_LENGTH);
    input_index = 0;
    current_state = PWD_STATE_IDLE;
}

/**
  * @brief  获取当前状态
  */
Password_State_t Password_GetState(void)
{
    return current_state;
}

/**
  * @brief  检查是否被锁定
  */
uint8_t Password_IsLocked(void)
{
    return security.is_locked;
}

/**
  * @brief  获取剩余锁定时间（秒）
  */
uint32_t Password_GetLockoutRemaining(void)
{
    if(!security.is_locked)
        return 0;
    
    uint32_t elapsed = HAL_GetTick() - security.lockout_start_tick;
    if(elapsed >= LOCKOUT_DURATION_MS)
        return 0;
    
    return (LOCKOUT_DURATION_MS - elapsed) / 1000;
}

/**
  * @brief  获取安全统计信息
  */
void Password_GetSecurityStats(Password_Security_t *stats)
{
    memcpy(stats, &security, sizeof(Password_Security_t));
}

/**
  * @brief  清空安全统计（需要物理访问）
  */
void Password_ClearSecurityStats(void)
{
    memset(&security, 0, sizeof(Password_Security_t));
    printf("[PWD] 安全统计已清空\n\r");
}
