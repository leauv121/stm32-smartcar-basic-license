#ifndef __MPU6050_H
#define __MPU6050_H

#include "main.h"

#define MPU6050_I2C_FLAG_TIMEOUT  10000

#define MPU6050_ADDRESS_LOW     0xD0U
#define MPU6050_ADDRESS_HIGH    0xD2U
#define MPU6050_ADDRESS         MPU6050_ADDRESS_LOW

/* MPU6050寄存器地址 */
#define MPU6050REG_SMPLRT_DIV       0x19
#define MPU6050REG_CONFIG           0x1A
#define MPU6050REG_GYRO_CONFIG      0x1B
#define MPU6050REG_ACCEL_CONFIG     0x1C
#define MPU6050REG_FIFO_EN			    0X23
#define MPU6050REG_INTBP_CFG		    0X37
#define MPU6050REG_INT_EN			      0X38
#define MPU6050REG_ACCEL_MEASURE    0x3B
#define MPU6050REG_TEMP_MEASURE     0x41
#define MPU6050REG_GYRO_MEASURE     0x43
#define MPU6050REG_USER_CTRL		    0X6A
#define MPU6050REG_PWR_MGMT_1       0x6B
#define MPU6050REG_PWR_MGMT_2		    0X6C
#define MPU6050REG_WHO_AM_I         0x75

/* MPU6050时钟源 */
#define MPU6050_CLOCK_INTERNAL          0x00
#define MPU6050_CLOCK_PLL_XGYRO         0x01
#define MPU6050_CLOCK_PLL_YGYRO         0x02
#define MPU6050_CLOCK_PLL_ZGYRO         0x03
#define MPU6050_CLOCK_PLL_EXT32K        0x04
#define MPU6050_CLOCK_PLL_EXT19M        0x05
#define MPU6050_CLOCK_KEEP_RESET        0x07

/* MPU6050陀螺仪量程 */
#define MPU6050_GYRO_FS_250         0x00
#define MPU6050_GYRO_FS_500         0x01
#define MPU6050_GYRO_FS_1000        0x02
#define MPU6050_GYRO_FS_2000        0x03

/* MPU6050加速度计量程 */
#define MPU6050_ACCEL_FS_2G          0x00
#define MPU6050_ACCEL_FS_4G          0x01
#define MPU6050_ACCEL_FS_8G          0x02
#define MPU6050_ACCEL_FS_16G         0x03

/* MPU6050 DMP库接口函数 */
uint8_t MPU6050_I2C_ReadRegister(unsigned char slave_addr,unsigned char reg_addr,unsigned short len, unsigned char *data_ptr);
uint8_t MPU6050_I2C_WriteRegister(unsigned char slave_addr,unsigned char reg_addr,unsigned short len,unsigned char *data_ptr);
uint8_t MPU6050_DetectAddress(void);
int get_tick_count(unsigned long *count);

void MPU6050_Init(void);
uint8_t MPU6050_ReadID(void);
void MPU6050_ReadAccel(int16_t *accelData);
void MPU6050_ReadGyro(int16_t *gyroData);
void MPU6050_GetTemp(float *Temperature);


#endif
