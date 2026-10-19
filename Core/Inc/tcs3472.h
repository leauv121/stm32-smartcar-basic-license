#ifndef MCU2025_I2C_TCS3472_H
#define MCU2025_I2C_TCS3472_H

#include "main.h"
#include "i2c.h"
/*******************************************************************************
 * 数据结构定义
*******************************************************************************/

// RGB颜色数据结构
typedef struct
{
    uint16_t c;  // Clear channel (无滤波器通道)
    uint16_t r;  // Red channel (红色通道)
    uint16_t g;  // Green channel (绿色通道)
    uint16_t b;  // Blue channel (蓝色通道)
} COLOR_RGBC;

// HSL颜色数据结构
typedef struct
{
    uint16_t h;  // Hue (色相) [0-360]
    uint8_t s;   // Saturation (饱和度) [0-100]
    uint8_t l;   // Lightness (亮度) [0-100]
} COLOR_HSL;

/*******************************************************************************
 * TCS34725寄存器地址定义
*******************************************************************************/
#define TCS34725_ADDRESS        (0x29 << 1)  // I2C地址已左移1位
#define TCS34725_COMMAND_BIT    0x80

#define TCS34725_ENABLE         0x00
#define TCS34725_ENABLE_PON     0x01    // Power ON
#define TCS34725_ENABLE_AEN     0x02    // RGBC Enable

#define TCS34725_ATIME          0x01    // Integration time register
#define TCS34725_CONTROL        0x0F    // Gain control register
#define TCS34725_ID             0x12    // ID register
#define TCS34725_STATUS         0x13    // Status register
#define TCS34725_STATUS_AVALID  0x01    // RGBC Valid

// 数据寄存器
#define TCS34725_CDATAL         0x14    // Clear channel data low byte
#define TCS34725_RDATAL         0x16    // Red channel data low byte
#define TCS34725_GDATAL         0x18    // Green channel data low byte
#define TCS34725_BDATAL         0x1A    // Blue channel data low byte

/*******************************************************************************
 * 积分时间配置值定义
*******************************************************************************/
#define TCS34725_INTEGRATIONTIME_2_4MS  0xFF    // 2.4ms - 1 cycle    - Max Count: 1024
#define TCS34725_INTEGRATIONTIME_24MS   0xF6    // 24ms  - 10 cycles  - Max Count: 10240
#define TCS34725_INTEGRATIONTIME_50MS   0xEB    // 50ms  - 20 cycles  - Max Count: 20480
#define TCS34725_INTEGRATIONTIME_101MS  0xD5    // 101ms - 42 cycles  - Max Count: 43008
#define TCS34725_INTEGRATIONTIME_154MS  0xC0    // 154ms - 64 cycles  - Max Count: 65535
#define TCS34725_INTEGRATIONTIME_700MS  0x00    // 700ms - 256 cycles - Max Count: 65535

/*******************************************************************************
 * 增益配置值定义
*******************************************************************************/
#define TCS34725_GAIN_1X    0x00    // 1x gain
#define TCS34725_GAIN_4X    0x01    // 4x gain
#define TCS34725_GAIN_16X   0x02    // 16x gain
#define TCS34725_GAIN_60X   0x03    // 60x gain

/*******************************************************************************
 * 函数声明 - 简化版本，只保留必要的接口
*******************************************************************************/

// TCS34725核心功能函数
uint8_t TCS34725_Init(void);
uint8_t TCS34725_GetRawData(COLOR_RGBC *rgbc);
void RGBtoHSL(COLOR_RGBC *Rgb, COLOR_HSL *Hsl);
void RGB_normalize(COLOR_RGBC *Rgb);
#endif //MCU2025_I2C_TCS3472_H
