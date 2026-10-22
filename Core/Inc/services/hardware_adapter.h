#ifndef FINAL_WORK_HARDWARE_ADAPTER_H
#define FINAL_WORK_HARDWARE_ADAPTER_H

#include <stdint.h>

typedef enum
{
    DRIVER_OK = 0,
    DRIVER_NOT_READY,
    DRIVER_TIMEOUT,
    DRIVER_ERROR
} DriverStatus;

typedef struct
{
    DriverStatus adc;
    DriverStatus mpu;
    DriverStatus color;
    DriverStatus ultrasonic;
    DriverStatus speed;
    DriverStatus battery;
    DriverStatus display;
} HardwareHealth;

/* 初始化函数集中调用旧驱动，应用状态机不直接依赖 HAL。 */
void HardwareAdapter_Init(void);
HardwareHealth HardwareAdapter_GetHealth(void);
void HardwareAdapter_SetRuntimeStatus(DriverStatus ultrasonic,
                                      DriverStatus speed,
                                      DriverStatus battery);

#endif
