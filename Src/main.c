/**
  ******************************************************************************
  * @file    main.c
  * @brief   智能私家车库系统 - 主程序
  * @note    基于 STM32F407 + HAL 库
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "tim.h"       // ← MX_TIM12_Init() 需要
#include "i2c.h"       // ← MX_I2C1_Init() 需要
#include "segment.h"   // ← Segment_Init() 需要
#include "servo.h"     // ← Servo_Init() 需要
#include "beep.h"
#include "usart.h"
#include "gpio.h"
#include "stdio.h"
#include "RemoteInfrared.h"
#include "password.h"
#include "adc.h"
#include "dma.h"
#include "light_ctrl.h"
#include "led.h"

/* External variables --------------------------------------------------------*/
extern __IO uint32_t FlagGotKey;
extern __IO Remote_Infrared_data_union RemoteInfrareddata;

/* Private variables ---------------------------------------------------------*/
__IO uint32_t GlobalTimingDelay100us;
/* 全局变量：ADC数据缓冲区 */
__IO uint16_t adcx[4] = {0};

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* Main function -------------------------------------------------------------*/
int main(void)
{
    /* MCU 初始化 */
    HAL_Init();
    SystemClock_Config();
    
    /* 外设初始化 */
    MX_GPIO_Init();      // 先初始化GPIO基础配置
    MX_DMA_Init();
    MX_ADC3_Init();
    MX_TIM12_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();

    /* 启动ADC DMA传输 */
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)adcx, 4);

    /* 模块初始化 */
    LED_Init();          // ← LED初始化移到这里
    Segment_Init(&hi2c1);
    Beep_Init();
    Servo_Init();
    LightCtrl_Init();
    Password_Init();

    /* 启动信息 */
    LED_Flow(100, 2);  // 启动时流水2次
    printf("\n\r========================================\n\r");
    printf("    智能私家车库系统 v1.0\n\r");
    printf("========================================\n\r");
    printf("[SYS] 系统初始化完成\n\r");
    printf("[SYS] 等待车辆到达...\n\r");
    
    // 测试模式：直接进入密码输入
    Password_StartInput();

    /* 主循环 */
    while(1)
    {
        /* 1. 心跳指示灯（系统运行指示） */
        LED_Heartbeat();

        /* 2. 光控任务（环境监测） */
        LightCtrl_Task();
        
        /* 3. 红外解码处理 */
        Remote_Infrared_KeyDeCode();
        
        /* 4. 检查是否有新按键 */
        if(FlagGotKey == 1)
        {
            FlagGotKey = 0;
            uint8_t keycode = RemoteInfrareddata.RemoteInfraredDataStruct.bKeyCode;
            Password_Process(keycode);
        }

        /* 5. 超时检查 */
        Password_TimeoutCheck();
        
        /* 6. 状态机处理 */
        Password_State_t state = Password_GetState();
        switch(state)
        {
            case PWD_STATE_SUCCESS:
                printf("[SYS] 密码正确，开门！\n\r");
                Beep_OK();      // ← 成功提示音
                Segment_DisplayOPEN();
                LED_Success();           // ← 成功指示灯
                Servo_Open();                    // 抬杆
                HAL_Delay(5000);                 // 保持5秒
                Servo_Close();                   // 落杆
                
                printf("[SYS] 闸机关闭\n\r");
                Password_Reset();
                Password_StartInput();
                break;
                
            case PWD_STATE_FAILED:
                printf("[SYS] 密码错误，报警！\n\r");
                Beep_Alarm();   // ← 报警音
                Segment_DisplayError();
                LED_Error();             // ← 错误指示灯
                HAL_Delay(2000);                 // 报警2秒
                Password_Reset();
                Password_StartInput();
                break;
                            
            case PWD_STATE_TIMEOUT:
                printf("[SYS] 输入超时，返回等待\n\r");
                Segment_DisplayTimeout();
                LED_FlashAll(200, 2);    // ← 超时闪烁
                HAL_Delay(2000);                 // 显示2秒
                Password_Reset();
                Password_StartInput();
                break;
                
            default:
                break;
        }
    }
}

/* System Clock Configuration */
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

/* Printf 重定向到串口 */
int fputc(int ch, FILE *f)
{ 
    while((USART1->SR & 0X40) == 0);
    USART1->DR = (uint8_t)ch;      
    return ch;
}

/* SysTick 回调 - 100us 定时基准 */
void HAL_SYSTICK_Callback(void)
{
    if(GlobalTimingDelay100us != 0)
    {
        GlobalTimingDelay100us--;
    }
}

/* GPIO 外部中断回调 - 红外接收 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    Remote_Infrared_KEY_ISR();
}

/************************ END OF FILE *****************************************/
