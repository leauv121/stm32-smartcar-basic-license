#ifndef __OLED_DRIVER_H
#define __OLED_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "soft_i2c.h"

#define OLED_I2C_ADDR 0x78

extern const uint8_t OLED_F6x8[][6];
extern const uint8_t OLED_F8x16[][16];
extern const uint8_t OLED_F16x16[][32];
extern uint8_t HZK[];
extern uint8_t BeiHang[];

void OLED_WriteCommand(uint8_t Command);
void OLED_WriteData(uint8_t Data);
void OLED_SetCursor(uint8_t Y, uint8_t X);
void OLED_Clear(void);
void OLED_Clear_Part(uint8_t Line, uint8_t start, uint8_t end);
void OLED_ShowChar(uint8_t X, uint8_t Y, uint8_t Char, uint8_t Char_Size);
void OLED_ShowString(uint8_t X, uint8_t Y, char *String, uint8_t Char_Size);
void OLED_ShowWord(uint8_t X, uint8_t Y, uint8_t ChineseIndex);
void OLED_ShowChinese(uint8_t X, uint8_t Y, uint8_t *Chinese, uint8_t Length);
uint32_t OLED_Pow(uint32_t X, uint32_t Y);
void OLED_ShowNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t X, uint8_t Y, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length);
void OLED_Init(void);

#ifdef __cplusplus
}
#endif

#endif
