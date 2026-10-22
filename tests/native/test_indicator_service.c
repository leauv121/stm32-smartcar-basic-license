#include "services/indicator_service.h"
#include "test_assert.h"

static void test_startup_pattern_flashes_three_times(void)
{
    IndicatorOutput output;

    IndicatorService_Init();
    IndicatorService_Start(INDICATOR_STARTUP_FLASH_3, 0U);

    output = IndicatorService_Update(0U);
    TEST_ASSERT_EQ_INT(1, output.left_on);
    TEST_ASSERT_EQ_INT(1, output.right_on);

    output = IndicatorService_Update(500U);
    TEST_ASSERT_EQ_INT(0, output.left_on);
    TEST_ASSERT_EQ_INT(0, output.right_on);

    output = IndicatorService_Update(2000U);
    TEST_ASSERT_EQ_INT(1, output.left_on);
    TEST_ASSERT_EQ_INT(1, output.right_on);

    output = IndicatorService_Update(3000U);
    TEST_ASSERT_EQ_INT(1, output.completed);
    TEST_ASSERT_EQ_INT(0, output.left_on);
    TEST_ASSERT_EQ_INT(0, output.right_on);
}

static void test_turn_and_hazard_patterns_use_required_intervals(void)
{
    IndicatorOutput output;

    IndicatorService_Start(INDICATOR_TURN_LEFT, 100U);
    output = IndicatorService_Update(399U);
    TEST_ASSERT_EQ_INT(1, output.left_on);
    TEST_ASSERT_EQ_INT(0, output.right_on);
    output = IndicatorService_Update(400U);
    TEST_ASSERT_EQ_INT(0, output.left_on);

    IndicatorService_Start(INDICATOR_HAZARD, 1000U);
    output = IndicatorService_Update(1499U);
    TEST_ASSERT_EQ_INT(1, output.left_on);
    TEST_ASSERT_EQ_INT(1, output.right_on);
    output = IndicatorService_Update(1500U);
    TEST_ASSERT_EQ_INT(0, output.left_on);
    TEST_ASSERT_EQ_INT(0, output.right_on);
}

int main(void)
{
    test_startup_pattern_flashes_three_times();
    test_turn_and_hazard_patterns_use_required_intervals();
    return test_failures;
}
