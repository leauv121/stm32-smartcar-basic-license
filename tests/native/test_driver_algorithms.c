#include "services/driver_algorithms.h"
#include "test_assert.h"

int main(void)
{
    ColorConfirmState color = {0};
    KeyDebounceState key = {0};

    TEST_ASSERT_TRUE((DriverAlgorithms_UltrasonicCm(1000U) > 16.99f) &&
                     (DriverAlgorithms_UltrasonicCm(1000U) < 17.01f));
    TEST_ASSERT_TRUE((DriverAlgorithms_SpeedRpm(1000000U, 1000000U) > 59.99f) &&
                     (DriverAlgorithms_SpeedRpm(1000000U, 1000000U) < 60.01f));
    TEST_ASSERT_TRUE(DriverAlgorithms_SpeedRpm(0U, 1000000U) == 0.0f);
    TEST_ASSERT_TRUE(!DriverAlgorithms_IsTimedOut(100U, 199U, 100U));
    TEST_ASSERT_TRUE(DriverAlgorithms_IsTimedOut(100U, 200U, 100U));

    TEST_ASSERT_EQ_INT(DRIVER_COLOR_NONE,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_RED, 3U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_NONE,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_RED, 3U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_RED, 3U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_NONE,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_GREEN, 3U));

    DriverAlgorithms_KeyInit(&key, 0U, 0U);
    TEST_ASSERT_TRUE(!DriverAlgorithms_KeyUpdate(&key, 1U, 5U, 20U));
    TEST_ASSERT_TRUE(!DriverAlgorithms_KeyUpdate(&key, 1U, 19U, 20U));
    TEST_ASSERT_TRUE(DriverAlgorithms_KeyUpdate(&key, 1U, 25U, 20U));
    TEST_ASSERT_TRUE(!DriverAlgorithms_KeyUpdate(&key, 1U, 40U, 20U));
    TEST_ASSERT_TRUE(!DriverAlgorithms_KeyUpdate(&key, 0U, 45U, 20U));
    TEST_ASSERT_TRUE(!DriverAlgorithms_KeyUpdate(&key, 0U, 70U, 20U));
    TEST_ASSERT_TRUE(!DriverAlgorithms_KeyUpdate(&key, 1U, 75U, 20U));
    TEST_ASSERT_TRUE(DriverAlgorithms_KeyUpdate(&key, 1U, 100U, 20U));

    return test_failures;
}
