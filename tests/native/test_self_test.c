#include "services/error_manager.h"
#include "services/self_test.h"
#include "test_assert.h"

static SelfTestInputs AllOk(void)
{
    SelfTestInputs input = {0};
    input.adc = DRIVER_OK;
    input.mpu = DRIVER_OK;
    input.color = DRIVER_OK;
    input.ultrasonic = DRIVER_OK;
    input.speed = DRIVER_OK;
    input.battery = DRIVER_OK;
    input.display = DRIVER_OK;
    input.color_enabled = 1U;
    input.ultrasonic_enabled = 1U;
    input.speed_enabled = 1U;
    input.battery_enabled = 1U;
    return input;
}

static void RunToEnd(const SelfTestInputs *input)
{
    uint32_t now_ms;
    for (now_ms = 1U; now_ms <= 10U; now_ms++)
    {
        (void)SelfTest_Update(input, now_ms);
    }
}

int main(void)
{
    SelfTestInputs input = AllOk();

    ErrorManager_Init();
    SelfTest_Start(0U);
    TEST_ASSERT_EQ_INT(SELF_TEST_ADC, SelfTest_GetState());
    RunToEnd(&input);
    TEST_ASSERT_TRUE(SelfTest_IsComplete());
    TEST_ASSERT_TRUE(SelfTest_IsSafeToRun());
    TEST_ASSERT_EQ_INT(SELF_TEST_COMPLETE, SelfTest_GetState());

    ErrorManager_Init();
    input = AllOk();
    input.color_enabled = 0U;
    input.ultrasonic_enabled = 0U;
    SelfTest_Start(0U);
    RunToEnd(&input);
    TEST_ASSERT_TRUE(SelfTest_IsSafeToRun());
    TEST_ASSERT_EQ_INT(SELF_TEST_SKIPPED, SelfTest_GetModuleResult(SELF_TEST_MODULE_COLOR));
    TEST_ASSERT_EQ_INT(SELF_TEST_SKIPPED, SelfTest_GetModuleResult(SELF_TEST_MODULE_ULTRASONIC));

    ErrorManager_Init();
    input = AllOk();
    input.color = DRIVER_ERROR;
    SelfTest_Start(0U);
    RunToEnd(&input);
    TEST_ASSERT_TRUE(SelfTest_IsComplete());
    TEST_ASSERT_TRUE(SelfTest_IsSafeToRun());
    TEST_ASSERT_EQ_INT(SELF_TEST_FAILED, SelfTest_GetModuleResult(SELF_TEST_MODULE_COLOR));
    TEST_ASSERT_EQ_INT(1, ErrorManager_GetActiveCount());

    ErrorManager_Init();
    input = AllOk();
    input.mpu = DRIVER_TIMEOUT;
    SelfTest_Start(0U);
    RunToEnd(&input);
    TEST_ASSERT_TRUE(SelfTest_IsComplete());
    TEST_ASSERT_TRUE(!SelfTest_IsSafeToRun());
    TEST_ASSERT_TRUE(ErrorManager_HasFatal());

    ErrorManager_Init();
    input = AllOk();
    input.ultrasonic = DRIVER_NOT_READY;
    SelfTest_Start(0U);
    (void)SelfTest_Update(&input, 1U);
    (void)SelfTest_Update(&input, 2U);
    (void)SelfTest_Update(&input, 3U);
    TEST_ASSERT_EQ_INT(SELF_TEST_ULTRASONIC, SelfTest_GetState());
    (void)SelfTest_Update(&input, 1500U);
    RunToEnd(&input);
    TEST_ASSERT_EQ_INT(SELF_TEST_FAILED,
                       SelfTest_GetModuleResult(SELF_TEST_MODULE_ULTRASONIC));
    TEST_ASSERT_TRUE(SelfTest_IsSafeToRun());

    return test_failures;
}
