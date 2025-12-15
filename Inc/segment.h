#ifndef __SEGMENT_H
#define __SEGMENT_H

#include "stm32f4xx_hal.h"
#include "zlg7290.h"

/* 数码管段码定义（共阴极编码） */
#define SEG_0    0xFC  // 0
#define SEG_1    0x0C  // 1
#define SEG_2    0xDA  // 2
#define SEG_3    0xF2  // 3
#define SEG_4    0x66  // 4
#define SEG_5    0xB6  // 5
#define SEG_6    0xBE  // 6
#define SEG_7    0xE0  // 7
#define SEG_8    0xFE  // 8
#define SEG_9    0xE6  // 9
#define SEG_A    0xEE  // A
#define SEG_B    0x3E  // b
#define SEG_C    0x9C  // C
#define SEG_D    0x7A  // d
#define SEG_E    0x9E  // E
#define SEG_F    0x8E  // F
#define SEG_H    0x6E  // H
#define SEG_L    0x1C  // L
#define SEG_N    0x2A  // n
#define SEG_O    0xFC  // O (same as 0)
#define SEG_P    0xCE  // P
#define SEG_R    0x0A  // r
#define SEG_S    0xB6  // S (same as 5)
#define SEG_T    0x1E  // t
#define SEG_U    0x7C  // U
#define SEG_MINUS 0x02 // -
#define SEG_OFF  0x00  // 熄灭

/* 函数声明 */
void Segment_Init(I2C_HandleTypeDef *hi2c);
void Segment_Clear(void);
void Segment_DisplayNumber(uint8_t pos, uint8_t num);
void Segment_DisplayString(const char *str);
void Segment_DisplayPassword(uint8_t count);
void Segment_DisplayPASS(void);
void Segment_DisplayOPEN(void);
void Segment_DisplayError(void);
void Segment_DisplayTimeout(void);

#endif /* __SEGMENT_H */
