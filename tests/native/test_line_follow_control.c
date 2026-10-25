#include "control/line_follow_control.h"
#include "test_assert.h"

static void test_centered_line_drives_both_wheels_equally(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2000U, 0U, 0U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(output.left_target, output.right_target);
    TEST_ASSERT_TRUE(output.left_target > 0);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
}

static void test_outer_sensors_request_rescue_turns(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2000U, 1U, 0U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    TEST_ASSERT_EQ_INT(4500, output.left_target);
    TEST_ASSERT_EQ_INT(1400, output.right_target);

    input.outer_right = 0U;
    input.outer_left = 1U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    TEST_ASSERT_EQ_INT(1400, output.left_target);
    TEST_ASSERT_EQ_INT(4500, output.right_target);
}

static void test_pid_derivative_uses_low_pass_filter(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2200U, 0U, 0U};

    LineFollowControl_Reset();
    LineFollowControl_Prime(2000U, 2000U);
    output = LineFollowControl_Update(input);

    /* 原车参数下：P=200，滤波后的D误差=100，Kd=0.5。 */
    TEST_ASSERT_EQ_INT(250, output.pid_output);
}

static void test_returning_from_rescue_resynchronizes_pid_history(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2000U, 1U, 0U};

    LineFollowControl_Reset();
    LineFollowControl_Prime(2000U, 2000U);
    (void)LineFollowControl_Update(input);

    input.left_adc = 1800U;
    input.right_adc = 2200U;
    input.outer_right = 0U;
    output = LineFollowControl_Update(input);

    /* 救线结束时以当前误差重新同步，避免产生微分突跳。 */
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
    TEST_ASSERT_EQ_INT(400, output.pid_output);
}

static void test_pid_output_is_clamped_to_safe_range(void)
{
    LineFollowOutput output;
    LineFollowInput input = {0U, 4095U, 0U, 0U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_TRUE(output.left_target <= 6500);
    TEST_ASSERT_TRUE(output.right_target >= 0);
}

int main(void)
{
    test_centered_line_drives_both_wheels_equally();
    test_outer_sensors_request_rescue_turns();
    test_pid_derivative_uses_low_pass_filter();
    test_returning_from_rescue_resynchronizes_pid_history();
    test_pid_output_is_clamped_to_safe_range();
    return test_failures;
}
