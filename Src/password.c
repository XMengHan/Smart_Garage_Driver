#include "password.h"
#include "stdio.h"

/* 正确密码 */
static const uint8_t CORRECT_PASSWORD[PASSWORD_LENGTH] = {1, 2, 3, 4, 5, 6};

/* 密码输入缓冲区 */
static uint8_t input_password[PASSWORD_LENGTH];
static uint8_t input_index = 0;

/* 状态管理 */
static Password_State_t current_state = PWD_STATE_IDLE;

/* 超时计时（毫秒） */
static uint32_t last_input_tick = 0;

/*******************************************************************************
 * @brief  键码转数字
 * @param  keycode: 红外键码
 * @retval 0-9 或 0xFF(无效)
 ******************************************************************************/
uint8_t IR_KeyCode_To_Number(uint8_t keycode)
{
    switch(keycode)
    {
        case IR_KEY_0: return 0;
        case IR_KEY_1: return 1;
        case IR_KEY_2: return 2;
        case IR_KEY_3: return 3;
        case IR_KEY_4: return 4;
        case IR_KEY_5: return 5;
        case IR_KEY_6: return 6;
        case IR_KEY_7: return 7;
        case IR_KEY_8: return 8;
        case IR_KEY_9: return 9;
        default: return 0xFF;
    }
}

/*******************************************************************************
 * @brief  初始化密码模块
 ******************************************************************************/
void Password_Init(void)
{
    Password_Reset();
}

/*******************************************************************************
 * @brief  开始输入密码
 ******************************************************************************/
void Password_StartInput(void)
{
    input_index = 0;
    current_state = PWD_STATE_INPUT;
    last_input_tick = HAL_GetTick();
    
    // 清空缓冲区
    for(int i = 0; i < PASSWORD_LENGTH; i++)
    {
        input_password[i] = 0xFF;
    }
    
    printf("\n\r[PWD] 请输入6位密码...\n\r");
}

/*******************************************************************************
 * @brief  处理红外输入
 * @param  ir_keycode: 红外键码
 ******************************************************************************/
void Password_Process(uint8_t ir_keycode)
{
    if(current_state != PWD_STATE_INPUT)
        return;
    
    // 更新超时计时
    last_input_tick = HAL_GetTick();
    
    // 处理删除键
    if(ir_keycode == IR_KEY_DEL)
    {
        if(input_index > 0)
        {
            input_index--;
            input_password[input_index] = 0xFF;
            printf("[PWD] 删除，当前已输入 %d 位\n\r", input_index);
        }
        return;
    }
    
    // 处理数字键
    uint8_t number = IR_KeyCode_To_Number(ir_keycode);
    if(number != 0xFF && input_index < PASSWORD_LENGTH)
    {
        input_password[input_index] = number;
        input_index++;
        printf("[PWD] 输入第 %d 位: *\n\r", input_index);  // 不显示具体数字
        
        // 检查是否输入完成
        if(input_index == PASSWORD_LENGTH)
        {
            // 校验密码
            uint8_t correct = 1;
            for(int i = 0; i < PASSWORD_LENGTH; i++)
            {
                if(input_password[i] != CORRECT_PASSWORD[i])
                {
                    correct = 0;
                    break;
                }
            }
            
            if(correct)
            {
                current_state = PWD_STATE_SUCCESS;
                printf("[PWD] >>> 密码正确！<<<\n\r");
            }
            else
            {
                current_state = PWD_STATE_FAILED;
                printf("[PWD] >>> 密码错误！<<<\n\r");
            }
        }
    }
}

/*******************************************************************************
 * @brief  获取当前状态
 ******************************************************************************/
Password_State_t Password_GetState(void)
{
    return current_state;
}

/*******************************************************************************
 * @brief  获取已输入位数
 ******************************************************************************/
uint8_t Password_GetInputCount(void)
{
    return input_index;
}

/*******************************************************************************
 * @brief  重置密码模块
 ******************************************************************************/
void Password_Reset(void)
{
    input_index = 0;
    current_state = PWD_STATE_IDLE;
    
    for(int i = 0; i < PASSWORD_LENGTH; i++)
    {
        input_password[i] = 0xFF;
    }
}

/*******************************************************************************
 * @brief  超时检查（在主循环中调用）
 ******************************************************************************/
void Password_TimeoutCheck(void)
{
    if(current_state == PWD_STATE_INPUT)
    {
        if((HAL_GetTick() - last_input_tick) > PASSWORD_TIMEOUT_MS)
        {
            current_state = PWD_STATE_TIMEOUT;
            printf("[PWD] >>> 输入超时！<<<\n\r");
        }
    }
}
