#ifndef __MPU6050_DMP_H
#define __MPU6050_DMP_H

#include "main.h"

uint8_t MPU6050_DMP_Init(void);
uint8_t MPU6050_DMP_GetData(float *pitch,float *roll,float *yaw);
uint8_t MPU6050_DMP_AccelGyroCalibration(void);

#endif
