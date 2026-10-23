#include "services/self_test.h"

#include "services/error_manager.h"

static SelfTestState state;
static SelfTestResult results[SELF_TEST_MODULE_COUNT];
static uint8_t safe_to_run;
static uint32_t state_started_ms;

#define SELF_TEST_STAGE_TIMEOUT_MS 1000U

static uint8_t IsEnabled(SelfTestModule module, const SelfTestInputs *inputs)
{
    switch (module)
    {
        case SELF_TEST_MODULE_COLOR: return inputs->color_enabled;
        case SELF_TEST_MODULE_ULTRASONIC: return inputs->ultrasonic_enabled;
        case SELF_TEST_MODULE_SPEED: return inputs->speed_enabled;
        case SELF_TEST_MODULE_BATTERY: return inputs->battery_enabled;
        default: return 1U;
    }
}

static DriverStatus GetStatus(SelfTestModule module, const SelfTestInputs *inputs)
{
    switch (module)
    {
        case SELF_TEST_MODULE_ADC: return inputs->adc;
        case SELF_TEST_MODULE_MPU: return inputs->mpu;
        case SELF_TEST_MODULE_COLOR: return inputs->color;
        case SELF_TEST_MODULE_ULTRASONIC: return inputs->ultrasonic;
        case SELF_TEST_MODULE_SPEED: return inputs->speed;
        case SELF_TEST_MODULE_BATTERY: return inputs->battery;
        case SELF_TEST_MODULE_DISPLAY: return inputs->display;
        default: return DRIVER_ERROR;
    }
}

static ErrorModule GetErrorModule(SelfTestModule module)
{
    static const ErrorModule map[SELF_TEST_MODULE_COUNT] =
    {
        ERR_MODULE_ADC, ERR_MODULE_MPU6050, ERR_MODULE_COLOR,
        ERR_MODULE_ULTRASONIC, ERR_MODULE_SPEED_LEFT,
        ERR_MODULE_BATTERY, ERR_MODULE_DISPLAY
    };
    return map[module];
}

static uint8_t IsFatalModule(SelfTestModule module)
{
    /* 循迹 ADC 与姿态 MPU 缺失时禁止动车，其余模块允许降级。 */
    return (uint8_t)((module == SELF_TEST_MODULE_ADC) ||
                     (module == SELF_TEST_MODULE_MPU));
}

void SelfTest_Start(uint32_t now_ms)
{
    uint8_t index;
    state = SELF_TEST_ADC;
    state_started_ms = now_ms;
    safe_to_run = 1U;
    for (index = 0U; index < (uint8_t)SELF_TEST_MODULE_COUNT; index++)
    {
        results[index] = SELF_TEST_PENDING;
    }
}

SelfTestState SelfTest_Update(const SelfTestInputs *inputs, uint32_t now_ms)
{
    SelfTestModule module;
    DriverStatus status;

    if ((inputs == 0) || (state == SELF_TEST_IDLE) ||
        (state == SELF_TEST_COMPLETE))
    {
        return state;
    }

    module = (SelfTestModule)((uint8_t)state - (uint8_t)SELF_TEST_ADC);
    if (IsEnabled(module, inputs) == 0U)
    {
        results[module] = SELF_TEST_SKIPPED;
        state = (SelfTestState)((uint8_t)state + 1U);
        state_started_ms = now_ms;
        return state;
    }

    status = GetStatus(module, inputs);
    if (status == DRIVER_NOT_READY)
    {
        if ((uint32_t)(now_ms - state_started_ms) < SELF_TEST_STAGE_TIMEOUT_MS)
        {
            return state;
        }
        status = DRIVER_TIMEOUT;
    }

    if (status == DRIVER_OK)
    {
        results[module] = SELF_TEST_PASSED;
    }
    else
    {
        ErrorCode code = (status == DRIVER_TIMEOUT) ?
                         ERROR_CODE_DATA_TIMEOUT : ERROR_CODE_INIT_FAILED;
        ErrorLevel level = (IsFatalModule(module) != 0U) ?
                           ERROR_LEVEL_FATAL : ERROR_LEVEL_WARNING;
        results[module] = SELF_TEST_FAILED;
        ErrorManager_Report(GetErrorModule(module), code, level, now_ms, 0U);
        if (level == ERROR_LEVEL_FATAL)
        {
            safe_to_run = 0U;
        }
    }

    state = (SelfTestState)((uint8_t)state + 1U);
    state_started_ms = now_ms;
    return state;
}

SelfTestState SelfTest_GetState(void)
{
    return state;
}

SelfTestResult SelfTest_GetModuleResult(SelfTestModule module)
{
    if (module >= SELF_TEST_MODULE_COUNT)
    {
        return SELF_TEST_FAILED;
    }
    return results[module];
}

uint8_t SelfTest_IsComplete(void)
{
    return (uint8_t)(state == SELF_TEST_COMPLETE);
}

uint8_t SelfTest_IsSafeToRun(void)
{
    return (uint8_t)((state == SELF_TEST_COMPLETE) && (safe_to_run != 0U));
}
