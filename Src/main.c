/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2015 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "gpio.h"
#include "i2c.h"
#include "zlg7290.h"

/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "RemoteInfrared.h"
#include "tim.h"
#include "adc.h"
#include "dma.h"

#define RELAY_PORT GPIOG
#define RELAY_PIN  GPIO_PIN_8

#define KEY_DEL 		 0x78
#define PASSWORD_LEN 8
#define DISP_LEN	 	 8
#define SEG_STAR 		 0x40
#define SERVO_CLOSE  600
#define SERVO_OPEN   2400
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
__IO uint32_t GlobalTimingDelay100us;
uint16_t adc_raw_data[4];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Seg_Display(uint8_t *buf);
void Password_Input(uint8_t num);
uint8_t Password_Check(void);
void Seg_Show_OPEN(void);
void Seg_Show_Err(void);
void Password_Reset(void);
void Password_Delete(void);
void Servo_Set(uint16_t pwm);
void LED_All_Off(void);
void LED_All_On(void);
void Turn_On_LED(uint8_t LED_NUM);
void Buzzer_Tone(uint32_t tone_delay, uint32_t duration_ms);
void Buzzer_Play_Melody(void);
void Relay_Init_GPIO(void);
void Relay_Control(uint8_t state);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
// 数码管转化
const uint8_t SEG_CODE_TABLE[16] =
{
    0xFC, // 0
    0x0C, // 1
    0xDA, // 2
    0xF2, // 3
    0x66, // 4
    0xB6, // 5
    0xBE, // 6
    0xE0, // 7
    0xFE, // 8
    0xE6, // 9

    0xEE, // A
    0x3E, // b
    0x9C, // C
    0x7A, // d
    0x00, // blank(?)
    0xFC  // all on
};
/* USER CODE BEGIN 0 */

typedef enum
{
    SYS_IDLE = 0,        // 待机
    SYS_INPUT_PWD,       // 输入密码ing
    SYS_VERIFY,          // 验证密码
    SYS_OPEN,            // 开门ing
    SYS_ERROR            // 密码错误
} SystemState_t;

SystemState_t SysState = SYS_IDLE;

/* 密码相关 */
uint8_t input_buf[DISP_LEN] = {14, 14, 14, 14, 14, 14};
uint8_t input_index = 0;
uint8_t password[PASSWORD_LEN] = {1,2,3,4,5,6,7,8};
uint8_t display_buf[PASSWORD_LEN] = {0};

uint32_t open_tick = 0;
uint32_t err_tick = 0;
uint32_t led_tick = 0;
uint8_t led_count = 0;

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
	MX_TIM12_Init();
	
	MX_I2C1_Init();
  MX_USART1_UART_Init();
	
	//MX_DMA_Init();
	//MX_ADC3_Init();
	//Relay_Init_GPIO();
	
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
	
	//HAL_ADC_Start_DMA(&hadc3, (uint32_t*)adc_raw_data, 4);

	uint8_t test_buf[8] = {14,14,14,14,14,14,14,14};

	Seg_Display(test_buf);
  /* USER CODE BEGIN 2 */
  printf("\n\r=================================================");
  printf("\n\r      ?? 基于 STM32F407 智能私家车库系统       ");
  printf("\n\r=================================================");
  printf("\n\r [系统功能就绪]");
  printf("\n\r 1. ? 门禁控制: 请使用红外遥控器输入密码");
  printf("\n\r    - 默认密码: 123456");
  printf("\n\r    - 按键: 0-9输入, CH-删除");
  printf("\n\r 2. ? 环境感知: 光敏传感器已启动");
  printf("\n\r    - 天黑自动开启车库照明 (继电器吸合)");
  printf("\n\r    - 天亮自动关闭车库照明");
  printf("\n\r-------------------------------------------------");
  printf("\n\r 系统初始化完成，正在运行中... \n\r");
  /* USER CODE END 2 */
	uint32_t light_check_tick = 0;
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
while (1)
{
    uint8_t key;

    key = Remote_Infrared_KeyDeCode();
	/*
	  if (HAL_GetTick() - light_check_tick > 500)
		{
				light_check_tick = HAL_GetTick();

				if (adc_raw_data[1] > 2000) // 
				{
						Relay_Control(1); // 
				}
				else
				{
						Relay_Control(0); //
				}
		}*/

    switch(SysState)
    {
        /* ================== 待机 ================== */
        case SYS_IDLE:
        {
						Servo_Set(SERVO_CLOSE);
						LED_All_Off();
					
            if (key <= 9)
            {
								Buzzer_Tone(2, 50);
                Password_Reset();          // 清空
                Password_Input(key);       // 输入第一位
                SysState = SYS_INPUT_PWD;
            }
        }
        break;

        /* ================== 输入密码 ================== */
        case SYS_INPUT_PWD:
        {
            if (key <= 9)
            {
                Password_Input(key);
								Buzzer_Tone(2, 50);

                /* 输入8位，校验 */
                if (input_index >= PASSWORD_LEN)
                {
                    SysState = SYS_VERIFY;
                } 
            } else if (key == KEY_DEL) {
								Buzzer_Tone(2, 50);
								Password_Delete();
						}
        }
        break;

        /* ================== 校验密码 ================== */
        case SYS_VERIFY:
        {
            if (Password_Check())
            {
                Seg_Show_OPEN();           // OPEN
								Buzzer_Play_Melody();
							
                open_tick = HAL_GetTick(); // 记录开门时间
								led_tick = HAL_GetTick();
								led_count = 0;
                SysState = SYS_OPEN;
            }
            else
            {
                Seg_Show_Err();            // Err
							  Buzzer_Tone(3, 1000);
							
                err_tick = HAL_GetTick();
                SysState = SYS_ERROR;
            }
        }
        break;

        /* ================== 开门 ================== */
        case SYS_OPEN:
        {
						Servo_Set(SERVO_OPEN);
            /* 不急  */
						if (HAL_GetTick() - led_tick >= 2000)
						{
								Turn_On_LED(led_count % 4);
								led_count++;
								led_tick = HAL_GetTick();
						}
					
            if (HAL_GetTick() - open_tick >= 50000)
            {
                /* 5s关门 */
								Servo_Set(SERVO_CLOSE);
								LED_All_Off();
                Password_Reset();
                SysState = SYS_IDLE;
            }
        }
        break;

        /* ================== 报警  ================== */
        case SYS_ERROR:
        {
						LED_All_On();
            /* 不急 */
            if (HAL_GetTick() - err_tick >= 20000)
            {
								LED_All_Off();
                Password_Reset();
                SysState = SYS_IDLE;
            }
        }
        break;

        default:
            SysState = SYS_IDLE;
            break;
    }
}


}

/** System Clock Configuration
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

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
	USART1->DR = (uint8_t) ch;      
	return ch;
}

void HAL_SYSTICK_Callback(void)
{
	if(GlobalTimingDelay100us != 0)
    {
       GlobalTimingDelay100us--;
    }
}

void Seg_Display(uint8_t *buf)
{
    uint8_t seg_buf[8];

    for (int i = 0; i < 8; i++)
    {
        if (buf[i] <= 15)
            seg_buf[i] = SEG_CODE_TABLE[buf[i]];
        else
            seg_buf[i] = buf[i];   // 灭
    }

    /*连续写6个字节*/
    I2C_ZLG7290_Write(&hi2c1, 0x70, 0x10, seg_buf, 8);
}

void Password_Input(uint8_t num)
{
    if (input_index < PASSWORD_LEN)
    {
        /* 1. 保存密码 */
        input_buf[input_index] = num;

        /* 2. * */
        display_buf[input_index] = SEG_STAR;

        input_index++;

        /* 3. 刷新  */
        Seg_Display(display_buf);
    }
}

uint8_t Password_Check(void)
{
    for (int i = 0; i < PASSWORD_LEN; i++)
    {
        if (input_buf[i] != password[i])
            return 0;
    }
    return 1;
}

void Seg_Show_OPEN(void)
{
    uint8_t buf[8] = {14,14,0xFC,0xCE,0x9E,0x2A,14,14};
    I2C_ZLG7290_Write(&hi2c1,0x70,0x10,buf,8);
}

void Seg_Show_Err(void)
{
    uint8_t buf[8] = {14,14,0x9E,0x0A,0x0A,14,14,14};
    I2C_ZLG7290_Write(&hi2c1,0x70,0x10,buf,8);
}

void Password_Reset(void)
{
    input_index = 0;

    for (int i = 0; i < DISP_LEN; i++){
        input_buf[i] = 0;
				display_buf[i] = 0;
		}
	
    Seg_Display(input_buf);
}

void Password_Delete(void)
{
    if (input_index > 0)
    {
        input_index--;
        input_buf[input_index] = 14;
				display_buf[input_index] = 14;

        /* 清空最后一位 */
        Seg_Display(display_buf);
    }
}

void Servo_Set(uint16_t pwm)
{
    __HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_1, pwm);
}

// 1.全关
void LED_All_Off(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_15, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0,  GPIO_PIN_SET);
}

// 2. 全开
void LED_All_On(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_15, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0,  GPIO_PIN_RESET);
}

// 3. 官方LED单开
void Turn_On_LED(uint8_t LED_NUM)
{

    LED_All_Off(); 

    switch(LED_NUM)
    {
        case 0:
            HAL_GPIO_WritePin(GPIOH, GPIO_PIN_15, GPIO_PIN_RESET); /* D4 */
            break;
        case 1:
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); /* D3 */
            break;
        case 2:
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0,  GPIO_PIN_RESET); /* D2 */
            break;
        case 3:
            HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET); /* D1 */
            break;          
        default:
            break;
    }
}

/* === 蜂鸣器控制函数 === */

// 产生特定频率和时间的声音
// tone_delay: 半周期延时 (单位: 100us). 推荐范围: 2~5
// duration_ms
void Buzzer_Tone(uint32_t tone_delay, uint32_t duration_ms)
{
    // 计数器翻转次数
    // 周期 = tone_delay * 2 * 100us
    // 总时间 = duration_ms * 1000us
    // 次数 = duration_ms * 10 / (tone_delay * 2)
    uint32_t toggle_counts = (duration_ms * 5) / tone_delay;

    for(uint32_t i = 0; i < toggle_counts; i++)
    {
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(tone_delay); // ??Delay???100us
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(tone_delay);
    }
}

// 开门旋律 (??? Do-Mi-So)
void Buzzer_Play_Melody(void)
{
    Buzzer_Tone(4, 150); // 低音
    HAL_Delay(500);      // 持续 (50ms)
    Buzzer_Tone(3, 150); // 中音
    HAL_Delay(500);
    Buzzer_Tone(2, 300); // 高音
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
		Remote_Infrared_KEY_ISR();
}

/* USER CODE BEGIN 4 */

// --- 继电器初始化 ---
void Relay_Init_GPIO(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1.  GPIOG 
    __HAL_RCC_GPIOG_CLK_ENABLE(); 

    // 2.  PG8
    GPIO_InitStruct.Pin = RELAY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // 推免输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RELAY_PORT, &GPIO_InitStruct);
    
    // 默认关闭
    HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_RESET);
}

// --- 继电器控制 ---
void Relay_Control(uint8_t state)
{
    if(state == 1)
        HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_SET);   // 开
    else
        HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_RESET); // 关
}
/* USER CODE END 4 */
/* USER CODE END 4 */

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
