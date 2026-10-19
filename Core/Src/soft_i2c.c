/**
 * @file soft_i2c.c
 * @brief 软件模拟 I2C 通信驱动(适用于任意 GPIO)
 * @note 支持标准 I2C 起始、停止、字节发送等基本操作
 */

#include "main.h"
#include "soft_i2c.h"

// 引脚定义(可配置)
#define SCL_Pin GPIO_PIN_5
#define SCL_GPIO_Port GPIOB
#define SDA_Pin GPIO_PIN_4
#define SDA_GPIO_Port GPIOB

// 快速切换 SCL/SDA 电平的宏
#define SCL_Set(x) HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, (GPIO_PinState)(x))
#define SDA_Set(x) HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, (GPIO_PinState)(x))

/**
 * @brief 初始化软件 I2C 所需的 GPIO(开漏输出，无上拉)
 */
void SoftI2C_Init(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE(); // 使能 GPIOB 时钟

    // 初始将 SCL 和 SDA 拉高(空闲状态)
    HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET);

    // 配置为开漏输出(OD),无内部上下拉,高速
    GPIO_InitStruct.Pin = SCL_Pin | SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SCL_GPIO_Port, &GPIO_InitStruct);

    // 再次确保总线空闲
    SCL_Set(1);
    SDA_Set(1);
}

/**
 * @brief 产生 I2C 起始条件:SCL 高时,SDA 从高变低
 */
void SoftI2C_Start(void) {
    SDA_Set(1);
    SCL_Set(1);
    SDA_Set(0);
    SCL_Set(0);
}

/**
 * @brief 产生 I2C 停止条件:SCL 高时,SDA 从低变高
 */
void SoftI2C_Stop(void) {
    SDA_Set(0);
    SCL_Set(1);
    SDA_Set(1);
}

/**
 * @brief 通过软件 I2C 发送一个字节(MSB 先发)
 * @param Byte 要发送的 8 位数据
 * @note 发送完后释放 SDA(拉高),并产生一个额外的时钟脉冲用于 ACK(此处未读 ACK,仅模拟)
 */
void SoftI2C_SendByte(uint8_t Byte) {
    for (uint8_t i = 0; i < 8; i++) {
        SDA_Set(Byte & (0x80 >> i));       // 发送第 i 位
        SCL_Set(1);                        // 时钟高电平,从设备采样
        SCL_Set(0);                        // 时钟低电平,主机准备下一位
    }
    // 释放 SDA 为 ACK 时钟做准备
    SDA_Set(1);
    SCL_Set(1); // 第 9 个时钟(ACK 位,此处不处理应答)
    SCL_Set(0);
}
