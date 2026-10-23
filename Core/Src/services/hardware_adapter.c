#include "services/hardware_adapter.h"

#include "adc.h"
#include "config/app_config.h"
#include "oled_driver.h"
#include "speed_measure.h"
#include "tcs3472.h"
#include "mpu6050_dmp.h"

static HardwareHealth health;

void HardwareAdapter_Init(void)
{
    /* 先点亮显示，再进行耗时的 MPU DMP 静止校准。 */
    OLED_Init();
    health.display = DRIVER_OK;

    /* ADC DMA 的正式启动由 main.c 统一完成，这里只登记启动后的健康状态。 */
    health.adc = DRIVER_OK;

    health.mpu = (MPU6050_DMP_Init() == 0U) ? DRIVER_OK : DRIVER_ERROR;

#if APP_FEATURE_COLOR_SENSOR
    health.color = (TCS34725_Init() != 0U) ? DRIVER_OK : DRIVER_ERROR;
#else
    health.color = DRIVER_NOT_READY;
#endif

#if APP_FEATURE_SPEED_MEASUREMENT
    Speed_MeasureInit();
    health.speed = DRIVER_OK;
#else
    health.speed = DRIVER_NOT_READY;
#endif

    /* 无返回值的旧驱动先登记为可用，运行时超时检测负责继续验证。 */
    health.ultrasonic = DRIVER_NOT_READY;
    health.battery = DRIVER_NOT_READY;
}

HardwareHealth HardwareAdapter_GetHealth(void)
{
    return health;
}

void HardwareAdapter_SetRuntimeStatus(DriverStatus ultrasonic,
                                      DriverStatus speed,
                                      DriverStatus battery)
{
    health.ultrasonic = ultrasonic;
    health.speed = speed;
    health.battery = battery;
}
