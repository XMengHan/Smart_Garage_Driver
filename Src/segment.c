#include "segment.h"
#include "string.h"

/* I2C 句柄 */
static I2C_HandleTypeDef *seg_hi2c = NULL;

/* ZLG7290 地址 */
#define ZLG_I2C_ADDR_WRITE  0x70
#define ZLG_I2C_ADDR_READ   0x71
#define ZLG_REG_DISP_START  0x10  // 显示缓冲区起始地址

/* 数字段码表 */
static const uint8_t NUM_TABLE[10] = {
    SEG_0, SEG_1, SEG_2, SEG_3, SEG_4,
    SEG_5, SEG_6, SEG_7, SEG_8, SEG_9
};

/*******************************************************************************
 * @brief  初始化数码管
 * @param  hi2c: I2C句柄指针
 ******************************************************************************/
void Segment_Init(I2C_HandleTypeDef *hi2c)
{
    seg_hi2c = hi2c;
    Segment_Clear();
}

/*******************************************************************************
 * @brief  清空显示
 ******************************************************************************/
void Segment_Clear(void)
{
    uint8_t clear_buf[8] = {0};
    I2C_ZLG7290_Write(seg_hi2c, ZLG_I2C_ADDR_WRITE, ZLG_REG_DISP_START, clear_buf, 8);
}

/*******************************************************************************
 * @brief  在指定位置显示单个数字
 * @param  pos: 位置 (0-7, 从左到右)
 * @param  num: 数字 (0-9)
 ******************************************************************************/
void Segment_DisplayNumber(uint8_t pos, uint8_t num)
{
    if(pos > 7 || num > 9) return;
    
    uint8_t seg_code = NUM_TABLE[num];
    I2C_ZLG7290_Write(seg_hi2c, ZLG_I2C_ADDR_WRITE, ZLG_REG_DISP_START + pos, &seg_code, 1);
}

/*******************************************************************************
 * @brief  显示字符串（支持数字和部分字母）
 * @param  str: 字符串（最多8个字符）
 ******************************************************************************/
void Segment_DisplayString(const char *str)
{
    uint8_t display_buf[8] = {0};
    uint8_t len = strlen(str);
    if(len > 8) len = 8;
    
    for(uint8_t i = 0; i < len; i++)
    {
        char c = str[i];
        
        if(c >= '0' && c <= '9')
            display_buf[i] = NUM_TABLE[c - '0'];
        else if(c >= 'A' && c <= 'F')
        {
            switch(c)
            {
                case 'A': display_buf[i] = SEG_A; break;
                case 'B': display_buf[i] = SEG_B; break;
                case 'C': display_buf[i] = SEG_C; break;
                case 'D': display_buf[i] = SEG_D; break;
                case 'E': display_buf[i] = SEG_E; break;
                case 'F': display_buf[i] = SEG_F; break;
            }
        }
        else if(c >= 'a' && c <= 'z')
        {
            switch(c)
            {
                case 'a': display_buf[i] = SEG_A; break;
                case 'b': display_buf[i] = SEG_B; break;
                case 'c': display_buf[i] = SEG_C; break;
                case 'd': display_buf[i] = SEG_D; break;
                case 'e': display_buf[i] = SEG_E; break;
                case 'f': display_buf[i] = SEG_F; break;
                case 'h': display_buf[i] = SEG_H; break;
                case 'l': display_buf[i] = SEG_L; break;
                case 'n': display_buf[i] = SEG_N; break;
                case 'o': display_buf[i] = SEG_O; break;
                case 'p': display_buf[i] = SEG_P; break;
                case 'r': display_buf[i] = SEG_R; break;
                case 's': display_buf[i] = SEG_S; break;
                case 't': display_buf[i] = SEG_T; break;
                case 'u': display_buf[i] = SEG_U; break;
            }
        }
        else if(c == '-')
            display_buf[i] = SEG_MINUS;
        else if(c == ' ')
            display_buf[i] = SEG_OFF;
    }
    
    I2C_ZLG7290_Write(seg_hi2c, ZLG_I2C_ADDR_WRITE, ZLG_REG_DISP_START, display_buf, 8);
}

/*******************************************************************************
 * @brief  显示密码输入进度（用横线表示）
 * @param  count: 已输入位数 (0-6)
 ******************************************************************************/
void Segment_DisplayPassword(uint8_t count)
{
    uint8_t display_buf[8] = {0};
    
    // 前6位用于显示密码进度
    for(uint8_t i = 0; i < 6; i++)
    {
        if(i < count)
            display_buf[i] = SEG_MINUS;  // 已输入显示 "-"
        else
            display_buf[i] = SEG_OFF;    // 未输入熄灭
    }
    
    I2C_ZLG7290_Write(seg_hi2c, ZLG_I2C_ADDR_WRITE, ZLG_REG_DISP_START, display_buf, 8);
}

/*******************************************************************************
 * @brief  显示 "PASS" (提示输入密码)
 ******************************************************************************/
void Segment_DisplayPASS(void)
{
    // PASS 显示为 "P A S S"，占用4位
    uint8_t display_buf[8] = {
        SEG_OFF, SEG_OFF,  // 前2位空白
        SEG_P, SEG_A, SEG_S, SEG_S,  // PASS
        SEG_OFF, SEG_OFF   // 后2位空白
    };
    
    I2C_ZLG7290_Write(seg_hi2c, ZLG_I2C_ADDR_WRITE, ZLG_REG_DISP_START, display_buf, 8);
}

/*******************************************************************************
 * @brief  显示 "OPEN" (开门)
 ******************************************************************************/
void Segment_DisplayOPEN(void)
{
    // OPEN 显示为 "O P E n"
    uint8_t display_buf[8] = {
        SEG_OFF, SEG_OFF,  // 前2位空白
        SEG_O, SEG_P, SEG_E, SEG_N,  // OPEN
        SEG_OFF, SEG_OFF   // 后2位空白
    };
    
    I2C_ZLG7290_Write(seg_hi2c, ZLG_I2C_ADDR_WRITE, ZLG_REG_DISP_START, display_buf, 8);
}

/*******************************************************************************
 * @brief  显示 "Err" (错误)
 ******************************************************************************/
void Segment_DisplayError(void)
{
    // Err 显示为 "E r r"
    uint8_t display_buf[8] = {
        SEG_OFF, SEG_OFF,  // 前2位空白
        SEG_E, SEG_R, SEG_R,  // Err
        SEG_OFF, SEG_OFF, SEG_OFF  // 后3位空白
    };
    
    I2C_ZLG7290_Write(seg_hi2c, ZLG_I2C_ADDR_WRITE, ZLG_REG_DISP_START, display_buf, 8);
}

/*******************************************************************************
 * @brief  显示 "timeout"
 ******************************************************************************/
void Segment_DisplayTimeout(void)
{
    // 显示 "timeout" 会比较困难，简化为 "tout"
    uint8_t display_buf[8] = {
        SEG_OFF, SEG_OFF,  // 前2位空白
        SEG_T, SEG_O, SEG_U, SEG_T,  // tout
        SEG_OFF, SEG_OFF   // 后2位空白
    };
    
    I2C_ZLG7290_Write(seg_hi2c, ZLG_I2C_ADDR_WRITE, ZLG_REG_DISP_START, display_buf, 8);
}
