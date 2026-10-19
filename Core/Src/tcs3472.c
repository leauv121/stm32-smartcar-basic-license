#include "tcs3472.h"

// TCS34725 I2C通信超时时间
#define TCS34725_I2C_TIMEOUT    100

// 默认配置参数
#define TCS34725_INTEGRATIONTIME_50MS   0xEB
#define TCS34725_GAIN_1X                0x00

/*******************************************************************************
 * @brief TCS34725初始化
 * @return uint8_t - 1:初始化成功, 0:初始化失败
*******************************************************************************/
uint8_t TCS34725_Init(void) {
    uint8_t id = 0;
    uint8_t cmd;
    HAL_StatusTypeDef status;

    // 读取设备ID
    status = HAL_I2C_Mem_Read(&hi2c1, TCS34725_ADDRESS, TCS34725_ID | TCS34725_COMMAND_BIT,
                              I2C_MEMADD_SIZE_8BIT, &id, 1, TCS34725_I2C_TIMEOUT);
    if (status != HAL_OK) return 0;

    // 检查设备ID (TCS34725:0x44, TCS34727:0x4D)
    if (id != 0x4D && id != 0x44) return 0;

    // 设置积分时间 (50ms)
    cmd = TCS34725_INTEGRATIONTIME_50MS;
    status = HAL_I2C_Mem_Write(&hi2c1, TCS34725_ADDRESS, TCS34725_ATIME | TCS34725_COMMAND_BIT,
                               I2C_MEMADD_SIZE_8BIT, &cmd, 1, TCS34725_I2C_TIMEOUT);
    if (status != HAL_OK) return 0;

    // 设置增益 (1x)
    cmd = TCS34725_GAIN_1X;
    status = HAL_I2C_Mem_Write(&hi2c1, TCS34725_ADDRESS, TCS34725_CONTROL | TCS34725_COMMAND_BIT,
                               I2C_MEMADD_SIZE_8BIT, &cmd, 1, TCS34725_I2C_TIMEOUT);
    if (status != HAL_OK) return 0;

    // 使能设备
    cmd = TCS34725_ENABLE_PON;
    status = HAL_I2C_Mem_Write(&hi2c1, TCS34725_ADDRESS, TCS34725_ENABLE | TCS34725_COMMAND_BIT,
                               I2C_MEMADD_SIZE_8BIT, &cmd, 1, TCS34725_I2C_TIMEOUT);
    if (status != HAL_OK) return 0;

    HAL_Delay(3);  // 等待上电

    cmd = TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN;
    status = HAL_I2C_Mem_Write(&hi2c1, TCS34725_ADDRESS, TCS34725_ENABLE | TCS34725_COMMAND_BIT,
                               I2C_MEMADD_SIZE_8BIT, &cmd, 1, TCS34725_I2C_TIMEOUT);
    if (status != HAL_OK) return 0;

    HAL_Delay(3);  // 等待RGBC使能

    return 1;
}

/*******************************************************************************
 * @brief TCS34725获取所有通道数据
 * @param rgbc - 指向COLOR_RGBC结构体的指针
 * @return uint8_t - 1:转换完成，数据可用; 0:转换未完成，数据不可用
*******************************************************************************/
uint8_t TCS34725_GetRawData(COLOR_RGBC *rgbc) {
    uint8_t status = 0;
    uint8_t data[8];  // 存储4个通道的数据，每个通道2字节
    HAL_StatusTypeDef hal_status;

    // 检查数据是否可用
    hal_status = HAL_I2C_Mem_Read(&hi2c1, TCS34725_ADDRESS, TCS34725_STATUS | TCS34725_COMMAND_BIT,
                                  I2C_MEMADD_SIZE_8BIT, &status, 1, TCS34725_I2C_TIMEOUT);
    if (hal_status != HAL_OK) return 0;

    if (status & TCS34725_STATUS_AVALID) {
        // 一次性读取所有通道数据（从CDATAL开始，连续读取8字节）
        hal_status = HAL_I2C_Mem_Read(&hi2c1, TCS34725_ADDRESS, TCS34725_CDATAL | TCS34725_COMMAND_BIT,
                                      I2C_MEMADD_SIZE_8BIT, data, 8, TCS34725_I2C_TIMEOUT);
        if (hal_status != HAL_OK) return 0;

        // 组合数据（小端序：低字节在前）
        rgbc->c = (data[1] << 8) | data[0];   // Clear
        rgbc->r = (data[3] << 8) | data[2];   // Red
        rgbc->g = (data[5] << 8) | data[4];   // Green
        rgbc->b = (data[7] << 8) | data[6];   // Blue

        return 1;
    }
    return 0;
}

/*******************************************************************************
 * @brief 求三个数中的最大值
 * @param a, b, c - 三个输入值
 * @return uint8_t - 最大值
*******************************************************************************/
static uint8_t max3v(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t max = a;
    if (b > max) max = b;
    if (c > max) max = c;
    return max;
}

/*******************************************************************************
 * @brief 求三个数中的最小值
 * @param a, b, c - 三个输入值
 * @return uint8_t - 最小值
*******************************************************************************/
static uint8_t min3v(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t min = a;
    if (b < min) min = b;
    if (c < min) min = c;
    return min;
}
//归一化RGB值
void RGB_normalize(COLOR_RGBC *Rgb) {
    
    if (Rgb->c == 0) return; // 防止除零错误
    Rgb->r = (Rgb->r * 255) / Rgb->c;
    Rgb->g = (Rgb->g * 255) / Rgb->c;
    Rgb->b = (Rgb->b * 255) / Rgb->c;
}
/*******************************************************************************
 * @brief RGB转HSL颜色空间转换
 * @param Rgb - 指向RGB颜色结构体的指针
 * @param Hsl - 指向HSL颜色结构体的指针
 * @return None
*******************************************************************************/
void RGBtoHSL(COLOR_RGBC *Rgb, COLOR_HSL *Hsl) {
    uint8_t maxVal, minVal, difVal;
    uint8_t r, g, b;

    // 防止除零错误
    if (Rgb->c == 0) {
        Hsl->h = 0;
        Hsl->s = 0;
        Hsl->l = 0;
        return;
    }

    r = Rgb->r * 100 / Rgb->c;   //[0-100]
    g = Rgb->g * 100 / Rgb->c;
    b = Rgb->b * 100 / Rgb->c;

    maxVal = max3v(r, g, b);
    minVal = min3v(r, g, b);
    difVal = maxVal - minVal;

    // 计算亮度
    Hsl->l = (maxVal + minVal) / 2;   //[0-100]

    if (maxVal == minVal) // 即r=g=b,灰度
    {
        Hsl->h = 0;
        Hsl->s = 0;
    } else {
        // 计算色相
        if (maxVal == r) {
            if (g >= b)
                Hsl->h = 60 * (g - b) / difVal;
            else
                Hsl->h = 60 * (g - b) / difVal + 360;
        } else if (maxVal == g) {
            Hsl->h = 60 * (b - r) / difVal + 120;
        } else if (maxVal == b) {
            Hsl->h = 60 * (r - g) / difVal + 240;
        }

        // 计算饱和度
        if (Hsl->l <= 50)
            Hsl->s = difVal * 100 / (maxVal + minVal);  //[0-100]
        else
            Hsl->s = difVal * 100 / (200 - (maxVal + minVal));
    }
}
