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
    TEST_ASSERT_TRUE(output.left_target > output.right_target);

    input.outer_right = 0U;
    input.outer_left = 1U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    TEST_ASSERT_TRUE(output.right_target > output.left_target);
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
    test_pid_output_is_clamped_to_safe_range();
    return test_failures;
}
