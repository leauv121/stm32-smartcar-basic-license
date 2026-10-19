#include "mpu6050_dmp.h"
#include <stdio.h>
#include <math.h>
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

#define q30  1073741824.0f

#define MPU6050_PowerOnCalibration 1

/* 陀螺仪方向控制矩阵 */
static signed char gyro_orientation[9] = { 1, 0, 0,
                                           0, 1, 0,
                                           0, 0, 1};

static  unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;

    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;      // error
    return b;
}

unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
  unsigned short scalar; 
  /*
     XYZ  010_001_000 Identity Matrix
     XZY  001_010_000
     YXZ  010_000_001
     YZX  000_010_001
     ZXY  001_000_010
     ZYX  000_001_010
  */
  scalar  = inv_row_2_scale(mtx);
  scalar |= inv_row_2_scale(mtx + 3) << 3;
  scalar |= inv_row_2_scale(mtx + 6) << 6;

  return scalar;
}

/**
* @brief MPU6050自校准函数，会以调用函数时MPU6050的姿态作为参考坐标系
* @param None
* @retval 0 测试通过 1 测试失败
*/
uint8_t MPU6050_DMP_AccelGyroCalibration(void)
{
	int result;
	long gyro[3], accel[3]; 
	result = mpu_run_self_test(gyro, accel);
	if (result == 0x03) 
	{
		/* Test passed. We can trust the gyro data here, so let's push it down
		 to the DMP.*/
		float sens;
		unsigned short accel_sens;
		mpu_get_gyro_sens(&sens);
		gyro[0] = (long)(gyro[0] * sens);
		gyro[1] = (long)(gyro[1] * sens);
		gyro[2] = (long)(gyro[2] * sens);
		dmp_set_gyro_bias(gyro);
		mpu_get_accel_sens(&accel_sens);
		accel[0] *= accel_sens;
		accel[1] *= accel_sens;
		accel[2] *= accel_sens;
		dmp_set_accel_bias(accel);
		return 0;
	}
	else
	{
	  return 1;
	}
}

/**
* @brief MPU6050 DMP初始化函数
* @param None
* @retval 0 DMP初始化成功 其他 DMP初始化失败
*/
uint8_t MPU6050_DMP_Init(void)
{
	uint8_t res;
	if(mpu_init()==0)//初始化MPU6050
	{	 
		res = mpu_set_sensors(INV_XYZ_GYRO|INV_XYZ_ACCEL);//设置所需要的传感器
		if(res)
		{
			return 1;
    }			
		res = mpu_configure_fifo(INV_XYZ_GYRO|INV_XYZ_ACCEL);//设置FIFO
		if(res)
		{
			return 2;
    }			
		res = mpu_set_sample_rate(DEFAULT_MPU_HZ);//设置采样率
		if(res)
		{
			return 3;
    }			
		res = dmp_load_motion_driver_firmware();//加载dmp固件
		if(res)
		{
			return 4;
    }			
		res = dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));//设置陀螺仪方向
		if(res)
		{
			return 5;
    }			
		res = dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT|DMP_FEATURE_TAP|//设置dmp功能
		    DMP_FEATURE_ANDROID_ORIENT|DMP_FEATURE_SEND_RAW_ACCEL|DMP_FEATURE_SEND_CAL_GYRO|
		    DMP_FEATURE_GYRO_CAL);
		if(res)
		{
			return 6;
		}
		res = dmp_set_fifo_rate(DEFAULT_MPU_HZ);//设置DMP输出速率(最大不超过200Hz)
		if(res)
		{
			return 7;
    }
#if MPU6050_PowerOnCalibration == 1
		res = MPU6050_DMP_AccelGyroCalibration();
		if(res)
		{
			return 8; 
    }	
#else
    long gyro[3]={0}, accel[3]={0};		
		res=mpu_run_self_test(gyro,accel);//自检
		if(res != 0x03)
		{
			return 8; 
    }	
#endif		
		/* PA8 配置为下降沿中断，因此将 MPU INT 设置为低电平有效。 */
		res = mpu_set_int_level(1);
		if(res)
		{
			return 11;
		}
		res = mpu_set_dmp_state(1);//使能DMP
		if(res)
		{
			return 9;
    }			
	}
	else
	{
	  return 10;
	}
	return 0;

}

/**
* @brief MPU6050获取DMP解算之后欧拉角
* @param pitch 俯仰角数据缓冲指针
* @param roll  横滚角数据缓冲指针
* @param yaw   航向角数据缓冲指针
* @retval 0 获取数据成功 1 获取数据失败
*/
uint8_t MPU6050_DMP_GetData(float *pitch,float *roll,float *yaw)
{
	short gyro[3], accel[3], sensors;
	unsigned long sensor_timestamp;
	unsigned char more = 0U;
	unsigned char packet_count = 0U;
	long quat[4];
	float q0, q1, q2, q3;
	float quaternion_norm;
	float pitch_sine;

	if ((pitch == 0) || (roll == 0) || (yaw == 0))
	{
		return 1;
	}

	/*
	 * FIFO 可能在 OLED、串口等任务执行期间积压多帧。
	 * 连续读取并使用最新一帧，避免一直显示过去的姿态。
	 * 上限 16 帧用于限制单次调用时间，防止主循环被长期占用。
	 */
	do
	{
		if (dmp_read_fifo(gyro, accel, quat,
		                  &sensor_timestamp, &sensors, &more))
		{
			return 1;
		}
		if ((sensors & INV_WXYZ_QUAT) == 0)
		{
			return 1;
		}
		packet_count++;
	} while ((more != 0U) && (packet_count < 16U));

	q0 = quat[0] / q30;
	q1 = quat[1] / q30;
	q2 = quat[2] / q30;
	q3 = quat[3] / q30;
	quaternion_norm = q0*q0 + q1*q1 + q2*q2 + q3*q3;
	if ((quaternion_norm < 0.8f) || (quaternion_norm > 1.2f))
	{
		return 1;
	}

	pitch_sine = -2.0f*q1*q3 + 2.0f*q0*q2;
	if (pitch_sine > 1.0f)
	{
		pitch_sine = 1.0f;
	}
	else if (pitch_sine < -1.0f)
	{
		pitch_sine = -1.0f;
	}

	*pitch = asinf(pitch_sine) * 57.2957795f;
	*roll  = atan2f(2.0f*q2*q3 + 2.0f*q0*q1,
	                1.0f - 2.0f*q1*q1 - 2.0f*q2*q2) * 57.2957795f;
	*yaw   = atan2f(2.0f*(q1*q2 + q0*q3),
	                q0*q0 + q1*q1 - q2*q2 - q3*q3) * 57.2957795f;
	return 0;
}
