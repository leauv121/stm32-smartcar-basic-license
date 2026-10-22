#ifndef FINAL_WORK_SELF_TEST_H
#define FINAL_WORK_SELF_TEST_H

#include <stdint.h>

#include "services/hardware_adapter.h"

typedef enum
{
    SELF_TEST_IDLE = 0,
    SELF_TEST_ADC,
    SELF_TEST_MPU,
    SELF_TEST_COLOR,
    SELF_TEST_ULTRASONIC,
    SELF_TEST_SPEED,
    SELF_TEST_BATTERY,
    SELF_TEST_DISPLAY,
    SELF_TEST_COMPLETE
} SelfTestState;

typedef enum
{
    SELF_TEST_MODULE_ADC = 0,
    SELF_TEST_MODULE_MPU,
    SELF_TEST_MODULE_COLOR,
    SELF_TEST_MODULE_ULTRASONIC,
    SELF_TEST_MODULE_SPEED,
    SELF_TEST_MODULE_BATTERY,
    SELF_TEST_MODULE_DISPLAY,
    SELF_TEST_MODULE_COUNT
} SelfTestModule;

typedef enum
{
    SELF_TEST_PENDING = 0,
    SELF_TEST_PASSED,
    SELF_TEST_SKIPPED,
    SELF_TEST_FAILED
} SelfTestResult;

typedef struct
{
    DriverStatus adc;
    DriverStatus mpu;
    DriverStatus color;
    DriverStatus ultrasonic;
    DriverStatus speed;
    DriverStatus battery;
    DriverStatus display;
    uint8_t color_enabled;
    uint8_t ultrasonic_enabled;
    uint8_t speed_enabled;
    uint8_t battery_enabled;
} SelfTestInputs;

void SelfTest_Start(uint32_t now_ms);
SelfTestState SelfTest_Update(const SelfTestInputs *inputs, uint32_t now_ms);
SelfTestState SelfTest_GetState(void);
SelfTestResult SelfTest_GetModuleResult(SelfTestModule module);
uint8_t SelfTest_IsComplete(void);
uint8_t SelfTest_IsSafeToRun(void);

#endif
