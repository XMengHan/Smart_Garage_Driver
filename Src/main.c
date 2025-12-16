/**
  ******************************************************************************
  * @file    main.c
  * @brief   智能私家车库系统 v2.0 - 主程序（安全加固版）
  * @note    基于 STM32F407 + HAL 库
  * 
  * @features
  *   - 完全模块化架构
  *   - 数据单元统一管理（system_data）
  *   - 硬件抽象层（hardware_interface）
  *   - 非阻塞延时工具（non_blocking）
  *   - 冷热启动标志处理
  *   - 状态机优化
  *   - 独立看门狗保护（10秒超时）
  *   - 密码安全机制（防暴力破解、时序攻击）
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "iwdg.h"                  // 看门狗头文件
#include "RemoteInfrared.h"
#include "password.h"
#include "light_ctrl.h"
#include "led.h"
#include "system_data.h"           // 数据单元
#include "hardware_interface.h"    // 硬件接口
#include "non_blocking.h"          // 非阻塞工具

/* External variables --------------------------------------------------------*/
extern __IO uint32_t FlagGotKey;
extern __IO Remote_Infrared_data_union RemoteInfrareddata;
extern __IO uint32_t GlobalTimingDelay100us;
extern IWDG_HandleTypeDef hiwdg;   // 看门狗句柄

/* 系统状态枚举 */
typedef enum {
    SYS_STATE_IDLE = 0,
    SYS_STATE_SUCCESS_OPEN,
    SYS_STATE_SUCCESS_WAIT,
    SYS_STATE_SUCCESS_CLOSE,
    SYS_STATE_ERROR,
    SYS_STATE_TIMEOUT,
    SYS_STATE_LOCKED         // ← 新增：系统锁定状态
} System_State_t;

/* Private variables ---------------------------------------------------------*/
static System_State_t system_state = SYS_STATE_IDLE;
static NonBlock_Delay_t state_delay = {0};
static NonBlock_Delay_t lockout_display_delay = {0};  // ← 锁定状态显示延时

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
void System_HandleLockout(void);

/**
  * @brief  主函数
  */
int main(void)
{
    /* ==================== 系统初始化 ==================== */
    
    /* MCU基础初始化 */
    HAL_Init();
    SystemClock_Config();
    
    /* 数据单元初始化（包含冷热启动检测） */
    SystemData_Init();
    
    /* 硬件接口初始化（包含看门狗） */
    HW_Init_All();
    
    printf("[SYS] 等待车辆到达...\n\r");
    
    /* 进入待机状态 */
    system_state = SYS_STATE_IDLE;
    Password_StartInput();

    /* ==================== 主循环 ==================== */
    while(1)
    {
        /* 任务0: 喂狗（防止系统复位） - 必须放在最前面！ */
        HAL_IWDG_Refresh(&hiwdg);
        
        /* 任务1: 心跳指示灯 */
        LED_Heartbeat();

        /* 任务2: 光控照明 */
        LightCtrl_Task();
        
        /* 任务3: 红外解码 */
        Remote_Infrared_KeyDeCode();
        
        /* 任务4: 按键处理 */
        if(FlagGotKey == 1)
        {
            FlagGotKey = 0;
            uint8_t keycode = RemoteInfrareddata.RemoteInfraredDataStruct.bKeyCode;
            
            /* 将按键数据保存到数据单元 */
            g_system.ir_key_flag = 1;
            g_system.ir_keycode = keycode;
            
            Password_Process(keycode);
        }

        /* 任务5: 超时检查 */
        Password_TimeoutCheck();
        
        /* 任务6: 状态机处理（非阻塞） */
        Password_State_t pwd_state = Password_GetState();
        
        switch(system_state)
        {
            /* ==================== 待机状态 ==================== */
            case SYS_STATE_IDLE:
                /* 检查是否进入锁定状态 */
                if(pwd_state == PWD_STATE_LOCKED)
                {
                    system_state = SYS_STATE_LOCKED;
                    NonBlock_Delay_Start(&lockout_display_delay, 5000);  // 5秒更新一次显示
                    break;
                }
                
                /* 检测密码状态变化 */
                if(pwd_state == PWD_STATE_SUCCESS)
                {
                    system_state = SYS_STATE_SUCCESS_OPEN;
                    SystemData_IncSuccessCount();  // 统计成功次数
                }
                else if(pwd_state == PWD_STATE_FAILED)
                {
                    system_state = SYS_STATE_ERROR;
                    SystemData_IncFailedCount();  // 统计失败次数
                }
                else if(pwd_state == PWD_STATE_TIMEOUT)
                {
                    system_state = SYS_STATE_TIMEOUT;
                    SystemData_IncTimeoutCount();  // 统计超时次数
                }
                break;
                
            /* ==================== 开门状态 ==================== */
            case SYS_STATE_SUCCESS_OPEN:
                printf("[SYS] 密码正确，开门！\n\r");
                
                /* 通过硬件接口层调用 */
                HW_Beep_OK();
                HW_Segment_ShowOPEN();
                HW_LED_Success();
                HW_Servo_Open();
                
                /* 启动非阻塞延时 */
                NonBlock_Delay_Start(&state_delay, 5000);
                
                system_state = SYS_STATE_SUCCESS_WAIT;
                break;
                
            /* ==================== 等待状态 ==================== */
            case SYS_STATE_SUCCESS_WAIT:
                /* 非阻塞等待5秒（期间持续喂狗，已在循环开头处理） */
                if(NonBlock_Delay_Check(&state_delay))
                {
                    system_state = SYS_STATE_SUCCESS_CLOSE;
                }
                break;
                
            /* ==================== 关门状态 ==================== */
            case SYS_STATE_SUCCESS_CLOSE:
                HW_Servo_Close();
                printf("[SYS] 闸机关闭\n\r");
                
                Password_Reset();
                Password_StartInput();
                system_state = SYS_STATE_IDLE;
                break;
                
            /* ==================== 错误状态 ==================== */
            case SYS_STATE_ERROR:
                printf("[SYS] 密码错误，报警！\n\r");
                
                HW_Beep_Alarm();
                HW_Segment_ShowError();
                HW_LED_Error();
                
                Password_Reset();
                Password_StartInput();
                system_state = SYS_STATE_IDLE;
                break;
                
            /* ==================== 超时状态 ==================== */
            case SYS_STATE_TIMEOUT:
                printf("[SYS] 输入超时\n\r");
                
                HW_Segment_ShowTimeout();
                HW_LED_FlashAll(200, 2);
                
                Password_Reset();
                Password_StartInput();
                system_state = SYS_STATE_IDLE;
                break;
                
            /* ==================== 锁定状态 ==================== */
            case SYS_STATE_LOCKED:
                System_HandleLockout();  // ← 处理锁定状态
                break;
                
            default:
                system_state = SYS_STATE_IDLE;
                break;
        }
    }
}

/**
  * @brief  处理系统锁定状态
  * @note   每5秒更新一次剩余时间显示
  */
void System_HandleLockout(void)
{
    /* 获取剩余锁定时间 */
    uint32_t remaining = Password_GetLockoutRemaining();
    
    /* 检查是否解除锁定 */
    if(remaining == 0)
    {
        printf("[SYS] 锁定解除，恢复正常\n\r");
        HW_Beep_Tick();
        Password_Reset();
        Password_StartInput();
        system_state = SYS_STATE_IDLE;
        return;
    }
    
    /* 定期更新显示 */
    if(NonBlock_Delay_Check(&lockout_display_delay))
    {
        printf("[SYS] ?? 系统锁定中，剩余 %lu 秒\n\r", remaining);
        
        /* 数码管显示 "Err" */
        HW_Segment_ShowError();
        
        /* LED 红色闪烁 */
        HW_LED_Error();
        
        /* 蜂鸣器短鸣提示 */
        HW_Beep_Tick();
        
        /* 重新启动延时 */
        NonBlock_Delay_Start(&lockout_display_delay, 5000);
    }
    
    /* 锁定期间拒绝任何输入 */
    if(FlagGotKey == 1)
    {
        FlagGotKey = 0;  // 清除按键标志
        HW_Beep_Alarm();  // 报警提示
        printf("[SYS] 系统已锁定，拒绝输入！剩余 %lu 秒\n\r", remaining);
    }
}

/**
  * @brief  系统时钟配置
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    __PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = 16;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);

    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/10000);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/**
  * @brief  错误处理函数
  * @note   当系统发生严重错误时调用，等待看门狗复位
  */
void Error_Handler(void)
{
    printf("[ERROR] 系统错误！等待看门狗复位...\n\r");
    
    /* 禁用中断 */
    __disable_irq();
    
    /* 停留在这里，等待看门狗复位系统 */
    while(1)
    {
        /* 不喂狗，10秒后自动复位 */
    }
}

/**
  * @brief  Printf 重定向到串口
  */
int fputc(int ch, FILE *f)
{ 
    while((USART1->SR & 0X40) == 0);
    USART1->DR = (uint8_t)ch;      
    return ch;
}

/**
  * @brief  SysTick 回调 - 100us 定时基准
  */
void HAL_SYSTICK_Callback(void)
{
    if(GlobalTimingDelay100us != 0)
    {
        GlobalTimingDelay100us--;
    }
}

/**
  * @brief  GPIO 外部中断回调 - 红外接收
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    Remote_Infrared_KEY_ISR();
}

/************************ END OF FILE *****************************************/
