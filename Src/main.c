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
#include "string.h"
#include "core_cm4.h"

#define RELAY_PORT GPIOG
#define RELAY_PIN  GPIO_PIN_8

#define KEY_DEL 		 0x78
#define PASSWORD_LEN 8
#define DISP_LEN	 	 8
#define SEG_STAR 		 0x40
#define SERVO_CLOSE  600
#define SERVO_OPEN   2400
#define AUTO_RESET_PERIOD_MS (2*1000) //自动复位周期
#define FLOW_TOKEN_VALID 0x96A53C21  //复杂魔术字

/* 数据备份宏 */
#define BKP_MAGIC_NUMBER  0xA5A5  // 数据是否有效魔术字
#define BKP_REG_MAGIC      RTC->BKP0R // reg0放魔术字
#define BKP_REG_PWD_1      RTC->BKP1R // reg1放密码前四位
#define BKP_REG_PWD_2      RTC->BKP2R // reg2放密码后四位
#define BKP_REG_STATE      RTC->BKP3R // 系统状态

#define BKP_REG_IDX      RTC->BKP4R  // 输入了几位input_index 
#define BKP_REG_INBUF_1  RTC->BKP5R  // input_buf 前4
#define BKP_REG_INBUF_2  RTC->BKP6R  // input_buf 后4

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
__IO uint32_t GlobalTimingDelay100us;
uint16_t adc_raw_data[4];
__IO uint32_t FlowSafetyToken = 0; //程序流安全令牌
/* USER CODE END PV */

/* USER CODE BEGIN PV */
IWDG_HandleTypeDef hiwdg;  // 看门狗句柄
/* USER CODE END PV */

/* USER CODE BEGIN PFP */
void MX_IWDG_Init(void);   // 声明初始化函数
/* USER CODE END PFP */

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
void SysData_Init(void);      // 初始化系统数据（恢复或重置）
void SysData_Save_PWD(void);  // 只保存密码
void SysData_Save_State(void);// 只保存状态
void System_Restore_Hardware(void); // 根据恢复状态，重置硬件
void SysData_Save_Input(void);     
void SysData_Restore_Input(void);
uint8_t Password_Check_Algorithm_A(void); // 算法A：正向XOR
uint8_t Password_Check_Algorithm_B(void); // 算法B：逆向减法
uint8_t SysData_Validate(void); // 数据校验


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

typedef struct
{
    uint8_t password[PASSWORD_LEN]; // 8字节密码
} SystemData_t;

// 全局数据实例
SystemData_t sysData; 

// 默认密码
const uint8_t DEFAULT_PASSWORD[PASSWORD_LEN] = {1,2,3,4,5,6,7,8};


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
uint8_t input_buf[DISP_LEN] = {14, 14, 14, 14, 14, 14, 14, 14};  //初始化全灭
uint8_t input_index = 0;
uint8_t display_buf[PASSWORD_LEN] = {0};

uint32_t open_tick = 0;
uint32_t err_tick = 0;
uint32_t led_tick = 0;
uint8_t led_count = 0;

/* USER CODE END 0 */


int main(void)
{
  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  // 必须初始化外设，否则时钟不开，硬件无法操作
  MX_GPIO_Init();
  
  // 【关键修复】GPIO Init后立即关灯，防止上电闪烁
  // 必须确认你的LED是低电平亮还是高电平亮，调用对应的Off函数
  LED_All_On(); 
	
  MX_TIM12_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
	


  // 【核心逻辑】先初始化硬件(默认状态)，再恢复数据并覆盖硬件状态
  // 这样如果是热启动，System_Restore_Hardware 会立即把灯/门改回正确的状态
  SysData_Init(); 
	
  // 初始化看门狗
  MX_IWDG_Init();
  HAL_IWDG_Refresh(&hiwdg);
  
  /* USER CODE BEGIN 2 */
  // 仅在冷启动或调试时打印，如果你想完全无感，可以把这些printf放到 SysData_Init 的冷启动分支里
  printf("\n\r=================================================");
  printf("\n\r       基于 STM32F407 智能私家车库系统       ");
  printf("\n\r=================================================");
  printf("\n\r [数据] 当前密码: ");
  for(int i=0;i<8;i++) printf("%d", sysData.password[i]);
  printf("\n\r [系统功能就绪]");
  printf("\n\r 1.  门禁控制: 请使用红外遥控器输入密码");
  printf("\n\r    - 按键: 0-9输入, CH-删除");
  printf("\n\r-------------------------------------------------");
  printf("\n\r 系统初始化完成，正在运行中... \n\r");
  /* USER CODE END 2 */

  while (1)
  {
      HAL_IWDG_Refresh(&hiwdg);
    
      if (HAL_GetTick() > AUTO_RESET_PERIOD_MS)
      {
          printf("\r\n [Safety] Scheduled Maintenance Reset triggered...");
          
          // 1. 强制保存全部关键数据
          SysData_Save_State();
          SysData_Save_Input();
          
          // 2. 延时，确保打印以及数据写入稳定
          HAL_Delay(100); 
          
          // 3. 执行复位
          NVIC_SystemReset(); 
      }

      uint8_t key;
      key = Remote_Infrared_KeyDeCode();

      switch(SysState)
      {
        /* ================== 待机 ================== */
        case SYS_IDLE:
        {
            FlowSafetyToken = 0;
            // 只有状态变化时才去操作硬件，避免循环里重复刷
            // 但为了安全，周期性刷也没问题
            Servo_Set(SERVO_CLOSE);
            LED_All_Off();
          
            if (key <= 9)
            {
                Buzzer_Tone(2, 50);
                Password_Reset();          // 清空
                Password_Input(key);       // 输入第一位
                SysState = SYS_INPUT_PWD;
                SysData_Save_State(); //状态变了备份
            }
        }
        break;

        /* ================== 输入密码 ================== */
        case SYS_INPUT_PWD:
        {
            FlowSafetyToken = 0;
            if (key <= 9)
            {
                Password_Input(key);
                Buzzer_Tone(2, 50);

                /* 输入8位，校验 */
                if (input_index >= PASSWORD_LEN)
                {
                    SysState = SYS_VERIFY;
                    SysData_Save_State(); //状态变了备份
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
                FlowSafetyToken = FLOW_TOKEN_VALID;
                Seg_Show_OPEN();           // OPEN
                Buzzer_Play_Melody();
              
                open_tick = HAL_GetTick(); // 记录开门时间
                led_tick = HAL_GetTick();
                led_count = 0;
                SysState = SYS_OPEN;
                SysData_Save_State(); //状态变了备份
            }
            else
            {
                FlowSafetyToken = 0;
              
                Seg_Show_Err();            // Err
                Buzzer_Tone(3, 1000);
              
                err_tick = HAL_GetTick();
                SysState = SYS_ERROR;
                SysData_Save_State(); //状态变了备份
            }
        }
        break;

        /* ================== 开门 ================== */
        case SYS_OPEN:
        {
            if (FlowSafetyToken == FLOW_TOKEN_VALID)
            {
                // 令牌正确
							  HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
                Servo_Set(SERVO_OPEN);
            }
            else 
            {
                // 令牌不对，CPU跳飞了或者有攻击
                Servo_Set(SERVO_CLOSE);
                printf("\r\n [FATAL] Flow Error! CPU PC JUMP DETECTED!");
                HAL_Delay(10);
                NVIC_SystemReset();
            }
              
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
                FlowSafetyToken = 0;
              
                Servo_Set(SERVO_CLOSE);
                LED_All_Off();
                Password_Reset();
                SysState = SYS_IDLE;
                SysData_Save_State(); //状态变了备份
            }
        }
        break;

        /* ================== 报警  ================== */
        case SYS_ERROR:
        {

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

/* ============================================================ */
/* ================ 数据单元与备份域核心实现 ================== */
/* ============================================================ */

/**
  * @brief  初始化系统数据。
  * 检查备份域是否有有效数据：
  * - 若有（热启动）：从备份域加载数据到RAM。
  * - 若无（冷启动/掉电）：加载默认值并写入备份域。
  */
/**
  * @brief  初始化系统数据。
  * 检查备份域是否有有效数据：
  * - 若有（热启动）：从备份域加载数据到RAM。
  * - 若无（冷启动/掉电）：加载默认值并写入备份域。
  */
/* USER CODE BEGIN 4 区域的实现 */
uint8_t SysData_Validate(void)
{
    // 1. 检查状态 (SysState) 是否在枚举范围内
    uint32_t state_val = BKP_REG_STATE;
    if (state_val > SYS_ERROR) return 0;

    // 2. 检查输入索引 (input_index) 是否越界
    uint32_t idx_val = BKP_REG_IDX;
    if (idx_val > PASSWORD_LEN) return 0;

    return 1; // 检查通过
}

void SysData_Init(void)
{
    // 1. 开启电源和备份域访问
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    
    // 【关键】防止 LED 在判断过程中闪烁，再次强制关闭
    LED_All_Off();

    // 2. 检查复位源
    uint8_t is_hot_start = 0;
    
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET || 
        __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET || 
        __HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
    {
        is_hot_start = 1; 
    }
    
    __HAL_RCC_CLEAR_RESET_FLAGS();

    // 3. 检查备份区数据 (双重验证：魔术字 + 内容合法性)
    if (BKP_REG_MAGIC == BKP_MAGIC_NUMBER && SysData_Validate() == 1 && is_hot_start == 1)
    {
        // === 数据有效 ===
        
        // 恢复密码
        uint32_t data1 = BKP_REG_PWD_1;
        uint32_t data2 = BKP_REG_PWD_2;
        sysData.password[0] = (uint8_t)(data1 >> 0);
        sysData.password[1] = (uint8_t)(data1 >> 8);
        sysData.password[2] = (uint8_t)(data1 >> 16);
        sysData.password[3] = (uint8_t)(data1 >> 24);
        sysData.password[4] = (uint8_t)(data2 >> 0);
        sysData.password[5] = (uint8_t)(data2 >> 8);
        sysData.password[6] = (uint8_t)(data2 >> 16);
        sysData.password[7] = (uint8_t)(data2 >> 24);

        if (is_hot_start)
        {
            // === 热启动逻辑：恢复状态，无延时 ===
            // printf("\r\n [System] Hot Start Detected!");
            
            // 恢复状态
            SysState = (SystemState_t)BKP_REG_STATE;
            
            // 恢复屏幕显示
            if (SysState == SYS_INPUT_PWD) 
            {
                SysData_Restore_Input();
            }
            else if (SysState == SYS_IDLE)
            {
                Password_Reset();
            }
            
            // 补发令牌
            if (SysState == SYS_OPEN)
            {
                FlowSafetyToken = FLOW_TOKEN_VALID; 
            }
            else
            {
                FlowSafetyToken = 0; 
            }

            // 【关键】立即恢复硬件状态 (覆盖掉 MX_GPIO_Init 的默认状态)
            System_Restore_Hardware();
        }
        else
        {
            SysState = SYS_IDLE; 
            BKP_REG_STATE = SYS_IDLE; 
            Password_Reset();

            HAL_Delay(100); 
        }
    }
    else
    {
        // === 冷启动 (无备份) ===
        printf("\r\n [System] Factory Reset.");
        HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
        // 加载默认密码
        memcpy(sysData.password, DEFAULT_PASSWORD, PASSWORD_LEN);
        SysData_Save_PWD(); 
        
        SysState = SYS_IDLE;
        SysData_Save_State(); 
        
        Password_Reset();
      
        printf("\r\n [System] Cold Start Delay (1s)...");
        HAL_Delay(1000); 
    }
}

// 保存密码 (只在修改密码时调用)
void SysData_Save_PWD(void)
{
    uint32_t data1 = 0, data2 = 0;
    
    data1 |= ((uint32_t)sysData.password[0] << 0);
    data1 |= ((uint32_t)sysData.password[1] << 8);
    data1 |= ((uint32_t)sysData.password[2] << 16);
    data1 |= ((uint32_t)sysData.password[3] << 24);
    
    data2 |= ((uint32_t)sysData.password[4] << 0);
    data2 |= ((uint32_t)sysData.password[5] << 8);
    data2 |= ((uint32_t)sysData.password[6] << 16);
    data2 |= ((uint32_t)sysData.password[7] << 24);
    
    BKP_REG_PWD_1 = data1;
    BKP_REG_PWD_2 = data2;
    BKP_REG_MAGIC = BKP_MAGIC_NUMBER;
}

// 保存状态 (在状态切换时调用)
void SysData_Save_State(void)
{
    BKP_REG_STATE = (uint32_t)SysState;
}


// 硬件恢复函数
void System_Restore_Hardware(void)
{
    switch(SysState)
    {
        case SYS_OPEN:
            Servo_Set(SERVO_OPEN); 
            open_tick = HAL_GetTick(); 
            // 恢复开门时的LED状态 (这里简单处理为继续跑马灯)
            break;
            
        case SYS_ERROR:
            LED_All_On(); 
            err_tick = HAL_GetTick();
            break;
            
        // 【关键】处理 IDLE, INPUT 等状态
        // 确保如果是热启动回到这些状态，硬件是关闭的
        default: 
            Servo_Set(SERVO_CLOSE);
            LED_All_Off();
            break;
    }
}
/* USER CODE BEGIN 4 */



/**
  * @brief 保存当前的输入缓存和进度到备份域
  * 每次按键输入或删除时都要调用
  */
void SysData_Save_Input(void)
{
    // 1. 保存索引
    BKP_REG_IDX = (uint32_t)input_index;
    
    // 2. 保存输入缓冲区 (input_buf)
    uint32_t data1 = 0, data2 = 0;
    
    // 压缩前4位
    data1 |= ((uint32_t)input_buf[0] << 0);
    data1 |= ((uint32_t)input_buf[1] << 8);
    data1 |= ((uint32_t)input_buf[2] << 16);
    data1 |= ((uint32_t)input_buf[3] << 24);
    
    // 压缩后4位
    data2 |= ((uint32_t)input_buf[4] << 0);
    data2 |= ((uint32_t)input_buf[5] << 8);
    data2 |= ((uint32_t)input_buf[6] << 16);
    data2 |= ((uint32_t)input_buf[7] << 24);
    
    BKP_REG_INBUF_1 = data1;
    BKP_REG_INBUF_2 = data2;
}

/**
  * @brief 从备份域恢复输入缓存，并刷新屏幕
  * 热启动时调用
  */
void SysData_Restore_Input(void)
{
    // 1. 恢复索引
    input_index = (uint8_t)BKP_REG_IDX;
    
    // 安全检查：如果索引乱了，就强制重置
    if (input_index > PASSWORD_LEN) input_index = 0;
    
    // 2. 恢复 buffer 数据
    uint32_t data1 = BKP_REG_INBUF_1;
    uint32_t data2 = BKP_REG_INBUF_2;
    
    input_buf[0] = (uint8_t)(data1 >> 0);
    input_buf[1] = (uint8_t)(data1 >> 8);
    input_buf[2] = (uint8_t)(data1 >> 16);
    input_buf[3] = (uint8_t)(data1 >> 24);
    
    input_buf[4] = (uint8_t)(data2 >> 0);
    input_buf[5] = (uint8_t)(data2 >> 8);
    input_buf[6] = (uint8_t)(data2 >> 16);
    input_buf[7] = (uint8_t)(data2 >> 24);
    
    // 3. 重建显示缓存 (display_buf) 并刷新屏幕
    // 逻辑：已经输入的位显示'*'，没输入的位显示'blank'(14)
    for (int i = 0; i < DISP_LEN; i++)
    {
        if (i < input_index)
        {
            display_buf[i] = SEG_STAR; // 已输入显示 *
        }
        else
        {
            display_buf[i] = 14;       // 未输入显示空
            input_buf[i] = 14;         // 确保 RAM 数据一致
        }
    }
    
    // 4. 立即刷新数码管
    Seg_Display(display_buf);
    
    printf("\r\n [Input] Restored: %d digits entered.", input_index);
}


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

    /*连续写8个字节*/
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
			
				SysData_Save_Input();
    }
}



uint8_t Password_Check_Algorithm_A(void)  //算法A：正向XOR
{
    uint8_t diff_accumulator = 0; // 差异累加器
    

    for (int i = 0; i < PASSWORD_LEN; i++)
    {
        diff_accumulator |= (input_buf[i] ^ sysData.password[i]);
    }
    
    // 如果 diff_accumulator 仍为 0,每一位都相同
    return (diff_accumulator == 0) ? 1 : 0;
}


uint8_t Password_Check_Algorithm_B(void)  //算法B：逆向减法
{
    uint8_t diff_accumulator = 0;
    
    for (int i = PASSWORD_LEN - 1; i >= 0; i--)
    {
        volatile uint8_t input_val = input_buf[i];
        volatile uint8_t pass_val  = sysData.password[i];
        
        // 累加差异
        diff_accumulator |= (input_val - pass_val);
    }
    
    return (diff_accumulator == 0) ? 1 : 0;
}


uint8_t Password_Check(void)  //随机AB算法进行检测
{
    uint32_t random_seed = HAL_GetTick();
    
    if (random_seed & 0x01) // 奇偶
    {
        printf("\r\n [Security] Verifying using Algo A (Forward XOR)...");
        return Password_Check_Algorithm_A();
    }
    else
    {
        printf("\r\n [Security] Verifying using Algo B (Reverse SUB)...");
        return Password_Check_Algorithm_B();
    }
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
				display_buf[i] = 14;
		}
	
    Seg_Display(display_buf);
		SysData_Save_Input();
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
				SysData_Save_Input();
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

// 3. 菇LED单开
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



void MX_IWDG_Init(void)
{
  hiwdg.Instance = IWDG;
  
  // LSI 频率 32kHz
  // 分频 64 -> 频率 = 32000 / 64 = 500Hz (每周期 2ms)
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64; 
  
  // 重装载值 1500 -> 超时时间 = 1500 * 2ms = 3000ms (3s)
  // 覆盖代码中 Buzzer 1s延时
  hiwdg.Init.Reload = 1500; 

  // 1. 初始化参数
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    while(1); // ???????
  }

  // 2. ????? (?? 0xCCCC ? KR ???)
  // ??:??????????????????!
  if (HAL_IWDG_Start(&hiwdg) != HAL_OK)
  {
    while(1); // 启动错误卡死
  }
}
/* USER CODE END 4 *

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
