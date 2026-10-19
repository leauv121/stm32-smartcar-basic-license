#include "mpu6050.h"
#include "i2c.h"

#define MPU6050_IIC_Handler hi2c1

/* HAL 使用左移一位后的 8 位地址：0x68->0xD0，0x69->0xD2。 */
static uint16_t mpu6050_active_address = MPU6050_ADDRESS_LOW;

uint8_t MPU6050_DetectAddress(void)
{
	if (HAL_I2C_IsDeviceReady(&MPU6050_IIC_Handler,
	                          MPU6050_ADDRESS_LOW,
	                          3U,
	                          20U) == HAL_OK)
	{
		mpu6050_active_address = MPU6050_ADDRESS_LOW;
		return 0x68U;
	}

	if (HAL_I2C_IsDeviceReady(&MPU6050_IIC_Handler,
	                          MPU6050_ADDRESS_HIGH,
	                          3U,
	                          20U) == HAL_OK)
	{
		mpu6050_active_address = MPU6050_ADDRESS_HIGH;
		return 0x69U;
	}

	return 0U;
}

/**
* @brief MPU6050 硬件I2C写入接口函数
* @param slave_addr 从机地址
* @param reg_addr   寄存器地址
* @param len        数据数量
* @param data_ptr   数据缓冲指针
* @retval status HAL_OK 写入正常 其他写入错误
*/
uint8_t MPU6050_I2C_WriteRegister(unsigned char slave_addr,unsigned char reg_addr,unsigned short len,unsigned char *data_ptr)
{
	(void)slave_addr;
	/* HAL_I2C_Mem_Write 是阻塞式接口，自身已经带超时。
	 * 通信失败时必须立即把错误返回给 DMP 初始化流程，不能无限等待设备上线。
	 */
	return (uint8_t)HAL_I2C_Mem_Write(&MPU6050_IIC_Handler,
	                                  mpu6050_active_address,
	                                  reg_addr,
	                                  I2C_MEMADD_SIZE_8BIT,
	                                  data_ptr,
	                                  len,
	                                  MPU6050_I2C_FLAG_TIMEOUT);
}

/**
* @brief MPU6050 硬件I2C读取接口函数
* @param slave_addr 从机地址
* @param reg_addr   寄存器地址
* @param len        数据数量
* @param data_ptr   数据缓冲指针
* @retval status HAL_OK 读取正常 其他 读取错误
*/
uint8_t MPU6050_I2C_ReadRegister(unsigned char slave_addr,unsigned char reg_addr,unsigned short len, unsigned char *data_ptr)
{
	(void)slave_addr;
	/* 与写接口相同：一次读取失败就返回错误，由上层决定初始化失败或重试。 */
	return (uint8_t)HAL_I2C_Mem_Read(&MPU6050_IIC_Handler,
	                                 mpu6050_active_address,
	                                 reg_addr,
	                                 I2C_MEMADD_SIZE_8BIT,
	                                 data_ptr,
	                                 len,
	                                 MPU6050_I2C_FLAG_TIMEOUT);
}

/**
* @brief MPU6050写寄存器
* @param reg_addr   寄存器地址
* @param data       写入的数据
* @retval None
*/
void MPU6050_WriteReg(uint8_t reg_addr, uint8_t data)
{
  MPU6050_I2C_WriteRegister(MPU6050_ADDRESS,reg_addr,1,&data);
}

/**
* @brief MPU6050读寄存器
* @param reg_addr   寄存器地址
* @param databuf    数据缓冲指针
* @param size       读取数量
* @retval None
*/
void MPU6050_ReadReg(uint8_t reg_addr, uint8_t *databuf,uint16_t size)
{
	MPU6050_I2C_ReadRegister(MPU6050_ADDRESS,reg_addr,size,databuf);
}

/**
* @brief MPU6050设置时钟源
* @param source   时钟源
* @retval None
*/
void MPU6050_SetClockSource(uint8_t source)
{
	uint8_t reg_temp;
	if(source <= MPU6050_CLOCK_KEEP_RESET)
	{
		/* 通过位运算防止覆盖寄存器非操作bit位的值 */
		MPU6050_ReadReg(MPU6050REG_PWR_MGMT_1,&reg_temp,1);
		reg_temp = (reg_temp&0xF8)|source;
		MPU6050_WriteReg(MPU6050REG_PWR_MGMT_1,reg_temp);
	}
}

/**
* @brief MPU6050设置陀螺仪量程
* @param range  量程
* @retval None
*/
void MPU6050_SetFullScaleGyroRange(uint8_t range)
{
	uint8_t reg_temp;
	if(range <= MPU6050_GYRO_FS_2000)
	{
		reg_temp = range << 3;
	  MPU6050_WriteReg(MPU6050REG_GYRO_CONFIG,reg_temp);
	}
}

/**
* @brief MPU6050设置加速度计量程
* @param range  量程
* @retval None
*/
void MPU6050_SetFullScaleAccelRange(uint8_t range)
{
	uint8_t reg_temp;
	if(range <= MPU6050_ACCEL_FS_16G)
	{
		reg_temp = range << 3;
	  MPU6050_WriteReg(MPU6050REG_ACCEL_CONFIG,reg_temp);	
	}
}

/**
* @brief MPU6050使能睡眠模式
* @param enabled 是否使能 1：使能 0：不使能
* @retval None
*/
void MPU6050_SetSleepEnabled(uint8_t enabled)
{
  uint8_t reg_temp;
	MPU6050_ReadReg(MPU6050REG_PWR_MGMT_1,&reg_temp,1);
	reg_temp = (reg_temp&0xBF)|(enabled << 6);
	MPU6050_WriteReg(MPU6050REG_PWR_MGMT_1,reg_temp);
}

/**
* @brief MPU6050设置采样率 
* @param rate 采样率 范围为4-1000Hz
* @retval None
*/
void MPU6050_SetSampleRate(uint16_t rate)
{
	uint8_t reg_temp;
	if(rate > 1000)
	{
		rate = 1000;
	}
	else if(rate < 4)
	{
		rate = 4;
	}
	/* 在启用了低通滤波器之后Fs=1kHz */
	reg_temp = 1000/rate - 1;
  MPU6050_WriteReg(MPU6050REG_SMPLRT_DIV,reg_temp);
}

/**
* @brief MPU6050设置低通滤波器频率 
* @param pbfreq 通带频率
* @retval None
*/
void MPU6050_SetLowPassFilter(uint16_t pbfreq)
{
	uint8_t reg_temp,data;
  if(pbfreq>=188)
	{
		data=1;
	}
	else if(pbfreq>=98)
	{
		data=2;
	}
	else if(pbfreq>=42)
	{
		data=3;
	}
	else if(pbfreq>=20)
	{
		data=4;
	}
	else if(pbfreq>=10)
	{
		data=5;
	}
	else
	{
		data=6;
	}
	MPU6050_ReadReg(MPU6050REG_CONFIG,&reg_temp,1);
	reg_temp = (reg_temp&0xF8)|(data);
	MPU6050_WriteReg(MPU6050REG_CONFIG,reg_temp);
}

/**
* @brief MPU6050使能DataReady中断 
* @param enabled 1 使能 0 禁用
* @retval None
*/
void MPU6050_SetDataReadyInterruptEnabled(uint8_t enabled)
{
  uint8_t reg_temp;
	MPU6050_ReadReg(MPU6050REG_INT_EN,&reg_temp,1);
	reg_temp = (reg_temp&0xFE)|enabled;
	MPU6050_WriteReg(MPU6050REG_INT_EN,reg_temp);
}

/**
* @brief MPU6050设置INT引脚有效电平 
* @param level 1 高电平 0 低电平
* @retval None
*/
void MPU6050_SetINTLevel(uint8_t level)
{
	uint8_t reg_temp;
	MPU6050_ReadReg(MPU6050REG_INTBP_CFG,&reg_temp,1);
	if(level)
	{
	  reg_temp = (reg_temp&0x7F)|(0<<7);
	}
	else
	{
		reg_temp = (reg_temp&0x7F)|(1<<7);
	}
	MPU6050_WriteReg(MPU6050REG_INTBP_CFG,reg_temp);
}

/**
* @brief MPU6050设置是否作为AUX I2C主机
* @param enabled 是否使能 1：使能 0：不使能
* @retval None
*/
void MPU6050_SetI2CMasterModeEnabled(uint8_t enabled)
{
	uint8_t reg_temp; 
	MPU6050_ReadReg(MPU6050REG_USER_CTRL,&reg_temp,1);
	reg_temp = (reg_temp&0xDF)|(enabled << 5);
	MPU6050_WriteReg(MPU6050REG_USER_CTRL,reg_temp);
}

/**
* @brief MPU6050设置使能主机直接访问AUX I2C
* @param enabled 是否使能 1：使能 0：不使能
* @retval None
*/
void MPU6050_SetI2CBypassEnabled(uint8_t enabled)
{
	uint8_t reg_temp;
	MPU6050_ReadReg(MPU6050REG_INTBP_CFG,&reg_temp,1);
	reg_temp = (reg_temp&0xFD)|(enabled << 1);
  MPU6050_WriteReg(MPU6050REG_INTBP_CFG,reg_temp);
}

/**
* @brief MPU6050软件复位
* @param None
* @retval None
*/
void MPU6050_Reset(void)
{
	MPU6050_WriteReg(MPU6050REG_PWR_MGMT_1, 0x80);
}

/**
* @brief MPU6050读取加速度计值
* @param accelData 数据缓冲指针
* @retval None
*/
void MPU6050_ReadAccel(int16_t *accelData)
{
    uint8_t buf[6];
    MPU6050_ReadReg(MPU6050REG_ACCEL_MEASURE, buf, 6);
    accelData[0] = (buf[0] << 8) | buf[1];
    accelData[1] = (buf[2] << 8) | buf[3];
    accelData[2] = (buf[4] << 8) | buf[5];
}

/**
* @brief MPU6050读取陀螺仪值
* @param gyroData 数据缓冲指针
* @retval None
*/
void MPU6050_ReadGyro(int16_t *gyroData)
{
    uint8_t buf[6];
    MPU6050_ReadReg(MPU6050REG_GYRO_MEASURE,buf,6);
    gyroData[0] = (buf[0] << 8) | buf[1];
    gyroData[1] = (buf[2] << 8) | buf[3];
    gyroData[2] = (buf[4] << 8) | buf[5];
}

/**
* @brief MPU6050读取温度值
* @param Temperature 数据缓冲指针
* @retval None
*/
void MPU6050_GetTemp(float * Temperature)
{
	int16_t temp3;
	uint8_t buf[2];
	MPU6050_ReadReg(MPU6050REG_TEMP_MEASURE,buf,2);     //读取温度值
  temp3= (buf[0] << 8) | buf[1];	
	*Temperature=((double) temp3/340.0)+36.53;
}

/**
* @brief MPU6050初始化
* @param None
* @retval None
*/
void MPU6050_Init(void)
{
	MPU6050_Reset();//复位
	HAL_Delay(100);
	MPU6050_SetSleepEnabled(0); //进入工作状态，在配置完之后再唤醒会导致配置的参数无效
  MPU6050_SetClockSource(MPU6050_CLOCK_PLL_XGYRO); //设置时钟
  MPU6050_SetFullScaleGyroRange(MPU6050_GYRO_FS_2000);//陀螺仪量程设置
  MPU6050_SetFullScaleAccelRange(MPU6050_ACCEL_FS_16G);	//加速度度最大量程 +-2G
	MPU6050_SetSampleRate(50);//设置采样率
	MPU6050_SetLowPassFilter(25);//设置低通滤波器频率
	MPU6050_SetI2CMasterModeEnabled(0);//不让MPU6050控制AUXI2C
	MPU6050_SetI2CBypassEnabled(0);	//主控制器的I2C与MPU6050的AUXI2C直通关闭
  MPU6050_SetDataReadyInterruptEnabled(1);//使能数据准备完成中断
  MPU6050_SetINTLevel(1);//设置INT引脚的有效电平为高电平	
}

/**
* @brief MPU6050读取器件ID，用于检测MPU6050连接是否正常
* @param None
* @retval 1 连接正常 0 连接异常
*/
uint8_t MPU6050_ReadID(void)
{
	unsigned char Re = 0;
    MPU6050_ReadReg(MPU6050REG_WHO_AM_I,&Re,1);    //读器件地址
	if(Re != 0x68)
	{
		return 0;
	}
	else
	{
		return 1;
	}
		
}

/**
* @brief DMP库获取时间接口函数
* @param count 数据缓冲指针
* @retval 0 
*/
int get_tick_count(unsigned long *count)
{
   count[0] = HAL_GetTick();
	return 0;
}
