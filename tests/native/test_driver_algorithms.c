#include "services/driver_algorithms.h"
#include "test_assert.h"

int main(void)
{
    ColorConfirmState color = {0};
    ColorConfirmState traffic_color = {0};
    KeyDebounceState key = {0};

    /* 原车四路循迹模块的 DO 输出：高电平表示检测到黑线。 */
    TEST_ASSERT_EQ_INT(0, DriverAlgorithms_LineDigitalIsBlack(0U));
    TEST_ASSERT_EQ_INT(1, DriverAlgorithms_LineDigitalIsBlack(1U));

    TEST_ASSERT_TRUE((DriverAlgorithms_UltrasonicCm(1000U) > 16.99f) &&
                     (DriverAlgorithms_UltrasonicCm(1000U) < 17.01f));
    TEST_ASSERT_TRUE((DriverAlgorithms_SpeedRpm(1000000U, 1000000U) > 59.99f) &&
                     (DriverAlgorithms_SpeedRpm(1000000U, 1000000U) < 60.01f));
    TEST_ASSERT_TRUE(DriverAlgorithms_SpeedRpm(0U, 1000000U) == 0.0f);
    TEST_ASSERT_TRUE(!DriverAlgorithms_IsTimedOut(100U, 199U, 100U));
    TEST_ASSERT_TRUE(DriverAlgorithms_IsTimedOut(100U, 200U, 100U));

    /* 低饱和度背景不能因色相抖动被误判成红牌或绿牌。 */
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_NONE,
                       DriverAlgorithms_ClassifyColor(120U, 29U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_NONE,
                       DriverAlgorithms_ClassifyColor(0U, 0U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ClassifyColor(0U, 70U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ClassifyColor(350U, 70U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_GREEN,
                       DriverAlgorithms_ClassifyColor(120U, 70U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_NONE,
                       DriverAlgorithms_ClassifyColor(60U, 70U));

    TEST_ASSERT_EQ_INT(DRIVER_COLOR_NONE,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_RED, 3U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_NONE,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_RED, 3U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_RED, 3U));
    /* 稳定保持红色时仍应持续返回红色，用于刷新传感器时间戳。 */
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_RED, 3U));
    /* 切换颜色期间保持上一个稳定结果，直到绿色连续确认。 */
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_GREEN, 3U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_GREEN, 3U));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_GREEN,
                       DriverAlgorithms_ConfirmColor(&color, DRIVER_COLOR_GREEN, 3U));

    /* 红牌两帧确认，绿牌五帧确认；绿色确认期间继续保持红牌状态。 */
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_NONE,
                       DriverAlgorithms_ConfirmTrafficColor(&traffic_color,
                                                            DRIVER_COLOR_RED));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmTrafficColor(&traffic_color,
                                                            DRIVER_COLOR_RED));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmTrafficColor(&traffic_color,
                                                            DRIVER_COLOR_GREEN));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmTrafficColor(&traffic_color,
                                                            DRIVER_COLOR_GREEN));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmTrafficColor(&traffic_color,
                                                            DRIVER_COLOR_GREEN));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_RED,
                       DriverAlgorithms_ConfirmTrafficColor(&traffic_color,
                                                            DRIVER_COLOR_GREEN));
    TEST_ASSERT_EQ_INT(DRIVER_COLOR_GREEN,
                       DriverAlgorithms_ConfirmTrafficColor(&traffic_color,
                                                            DRIVER_COLOR_GREEN));

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
