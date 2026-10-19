/**
 * @file oled_driver.c
 * @brief 基于 SSD1306 的 128x64 OLED 显示驱动(使用软件 I2C)
 * @note 支持字符、字符串、数字、汉字(需字模数组)
 */

#include "main.h"
#include "string.h"
#include "soft_i2c.h"
#include "oled_driver.h"
#include "oledfont.h"

uint8_t HZK[] = {0, 1, 2, 3, 4, 5, 6};
uint8_t BeiHang[] = {7, 8};

const uint8_t OLED_F16x16[][32] = {
    /* 我 */
    {
        0x20,0x24,0x24,0x24,0xfe,0x23,0x22,0x20,0xff,0x20,0x22,0xac,0x20,0x30,0x20,0x00,
        0x00,0x08,0x48,0x84,0x7f,0x02,0x21,0x10,0x09,0x06,0x1a,0x61,0x80,0xe0,0x00,0x00
    },
    /* 在 */
    {
        0x08,0x08,0x08,0x08,0xc8,0x38,0x0f,0x08,0x08,0xe8,0x08,0x88,0x08,0x0c,0x08,0x00,
        0x08,0x04,0x02,0xff,0x00,0x40,0x41,0x41,0x41,0x7f,0x41,0x41,0x41,0x60,0x40,0x00
    },
    /* 教 */
    {
        0x20,0x24,0x24,0xa4,0xff,0xa4,0xb4,0xac,0x20,0x9f,0x10,0x10,0xf0,0x18,0x10,0x00,
        0x02,0x12,0x51,0x90,0x7e,0x0a,0x89,0x40,0x20,0x1b,0x04,0x1b,0x60,0xc0,0x40,0x00
    },
    /* 室 */
    {
        0x10,0x0c,0x24,0x24,0xa4,0x64,0x25,0x26,0x24,0x24,0xa4,0x34,0x24,0x14,0x0c,0x00,
        0x00,0x40,0x40,0x49,0x49,0x49,0x49,0x7f,0x49,0x49,0x49,0x4b,0x60,0x40,0x00,0x00
    },
    /* 很 */
    {
        0x40,0x20,0x10,0x8c,0x63,0x00,0xfe,0x92,0x92,0x92,0x92,0x92,0xff,0x02,0x00,0x00,
        0x04,0x02,0x01,0xff,0x00,0x00,0xff,0x40,0x23,0x04,0x08,0x14,0x62,0xc3,0x40,0x00
    },
    /* 想 */
    {
        0x08,0x88,0x68,0xff,0x48,0x88,0x08,0xfe,0x52,0x52,0x52,0x52,0xff,0x02,0x00,0x00,
        0x41,0x30,0x00,0x7b,0x80,0x80,0x88,0x93,0xb2,0x82,0x82,0xe2,0x03,0x10,0x60,0x00
    },
    /* 你 */
    {
        0x40,0x20,0xf8,0x07,0x40,0x20,0x18,0x0f,0x08,0xc8,0x08,0x08,0x28,0x18,0x00,0x00,
        0x00,0x00,0xff,0x00,0x00,0x08,0x04,0x43,0x80,0x7f,0x00,0x01,0x06,0x0c,0x00,0x00
    },
    /* 北 */
    {
        0x00,0x20,0x20,0x20,0x20,0xFF,0x00,0x00,0x00,0xFF,0x40,0x20,0x10,0x08,0x00,0x00,
        0x20,0x60,0x20,0x10,0x10,0xFF,0x00,0x00,0x00,0x3F,0x40,0x40,0x40,0x40,0x78,0x00
    },
    /* 航 */
    {
        0x80,0xFC,0x96,0xE5,0x84,0xFC,0x00,0x08,0xC8,0x49,0x4A,0xC8,0x08,0x08,0x00,0x00,
        0x80,0x7F,0x02,0x4C,0x80,0x7F,0x80,0x60,0x1F,0x00,0x00,0x3F,0x40,0x40,0x78,0x00
    }
};

void OLED_WriteCommand(uint8_t Command) {
    SoftI2C_Start();
    SoftI2C_SendByte(OLED_I2C_ADDR);
    SoftI2C_SendByte(0x00);
    SoftI2C_SendByte(Command);
    SoftI2C_Stop();
}

void OLED_WriteData(uint8_t Data) {
    SoftI2C_Start();
    SoftI2C_SendByte(OLED_I2C_ADDR);
    SoftI2C_SendByte(0x40);
    SoftI2C_SendByte(Data);
    SoftI2C_Stop();
}

void OLED_SetCursor(uint8_t Y, uint8_t X) {
    OLED_WriteCommand(0xB0 | Y);
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));
    OLED_WriteCommand(0x00 | (X & 0x0F));
}

void OLED_Clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        OLED_SetCursor(page, 0);
        for (uint8_t col = 0; col < 128; col++) {
            OLED_WriteData(0x00);
        }
    }
}

void OLED_Clear_Part(uint8_t Line, uint8_t start, uint8_t end) {
    for (uint8_t Column = start; Column <= end; Column++) {
        OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
        for (uint8_t i = 0; i < 8; i++) {
            OLED_WriteData(0x00);
        }
        OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
        for (uint8_t i = 0; i < 8; i++) {
            OLED_WriteData(0x00);
        }
    }
}

void OLED_ShowChar(uint8_t X, uint8_t Y, uint8_t Char, uint8_t Char_Size) {
    uint8_t c = 0, i = 0;
    c = Char - ' ';
    if (X > 128 - 1) {
        X = 0;
        Y = Y + 2;
    }
    if (Char_Size <= 16) {
        OLED_SetCursor(Y, X);
        for (i = 0; i < 8; i++) {
            OLED_WriteData(OLED_F8x16[c][i]);
        }
        OLED_SetCursor(Y + 1, X);
        for (i = 0; i < 8; i++) {
            OLED_WriteData(OLED_F8x16[c][i + 8]);
        }
    } else {
        OLED_SetCursor(Y, X);
        for (i = 0; i < 6; i++) {
            OLED_WriteData(OLED_F6x8[c][i]);
        }
    }
}

void OLED_ShowString(uint8_t X, uint8_t Y, char *String, uint8_t Char_Size) {
    uint8_t j = 0;
    while (String[j] != '\0') {
        OLED_ShowChar(X, Y, (uint8_t)String[j], Char_Size);
        X += 8;
        if (X > 120) {
            X = 0;
            Y += 2;
        }
        j++;
    }
}

void OLED_ShowWord(uint8_t X, uint8_t Y, uint8_t ChineseIndex) {
    const uint8_t *font = OLED_F16x16[ChineseIndex];
    OLED_SetCursor(Y, X);
    for (uint8_t i = 0; i < 8; i++) {
        OLED_WriteData(font[i]);
    }
    OLED_SetCursor(Y + 1, X);
    for (uint8_t i = 0; i < 8; i++) {
        OLED_WriteData(font[i + 8]);
    }
    OLED_SetCursor(Y, X + 8);
    for (uint8_t i = 0; i < 8; i++) {
        OLED_WriteData(font[i + 16]);
    }
    OLED_SetCursor(Y + 1, X + 8);
    for (uint8_t i = 0; i < 8; i++) {
        OLED_WriteData(font[i + 24]);
    }
}

void OLED_ShowChinese(uint8_t X, uint8_t Y, uint8_t *Chinese, uint8_t Length) {
    for (uint8_t i = 0; i < Length; i++) {
        OLED_ShowWord(X + i * 16, Y, Chinese[i]);
    }
}

uint32_t OLED_Pow(uint32_t X, uint32_t Y) {
    uint32_t result = 1;
    while (Y--) {
        result *= X;
    }
    return result;
}

void OLED_ShowNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length) {
    for (uint8_t i = 0; i < Length; i++) {
        uint8_t digit = (Number / OLED_Pow(10, Length - i - 1)) % 10;
        OLED_ShowChar(X + i * 8, Y, digit + '0', 16);
    }
}

void OLED_ShowSignedNum(uint8_t X, uint8_t Y, int32_t Number, uint8_t Length) {
    if (Number >= 0) {
        OLED_ShowChar(X, Y, '+', 16);
        OLED_ShowNum(X + 8, Y, (uint32_t)Number, Length);
    } else {
        OLED_ShowChar(X, Y, '-', 16);
        OLED_ShowNum(X + 8, Y, (uint32_t)(-Number), Length);
    }
}

void OLED_ShowHexNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length) {
    for (uint8_t i = 0; i < Length; i++) {
        uint8_t digit = (Number >> (4 * (Length - i - 1))) & 0x0F;
        uint8_t ch = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        OLED_ShowChar(X + i * 8, Y, ch, 16);
    }
}

void OLED_ShowBinNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length) {
    for (uint8_t i = 0; i < Length; i++) {
        uint8_t bit = (Number >> (Length - i - 1)) & 1;
        OLED_ShowChar(X + i * 8, Y, bit + '0', 16);
    }
}

void OLED_Init(void) {
    for (volatile uint32_t i = 0; i < 1000000; i++);

    OLED_WriteCommand(0xAE);
    OLED_WriteCommand(0xD5);
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8);
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0xA1);
    OLED_WriteCommand(0xC8);
    OLED_WriteCommand(0xDA);
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81);
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9);
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB);
    OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4);
    OLED_WriteCommand(0xA6);
    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);

    OLED_Clear();
}
