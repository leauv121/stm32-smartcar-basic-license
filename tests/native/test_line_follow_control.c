#include "control/line_follow_control.h"
#include "config/car_parameters.h"
#include "test_assert.h"

static void test_centered_inner_sensors_drive_straight(void)
{
    LineFollowInput input = {3500U, 3500U, 0U, 0U, 1U};
    LineFollowOutput output;

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);

    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
    TEST_ASSERT_EQ_INT(0, output.pid_output);
    TEST_ASSERT_EQ_INT(output.left_target, output.right_target);
}

static void test_right_inner_black_turns_vehicle_right(void)
{
    LineFollowInput input = {150U, 3500U, 0U, 0U, 1U};
    LineFollowOutput output;

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);

    TEST_ASSERT_TRUE(output.pid_output > 0);
    TEST_ASSERT_TRUE(output.left_target > output.right_target);
}

static void test_outer_probe_has_more_authority_than_inner_probe(void)
{
    LineFollowInput inner = {150U, 3500U, 0U, 0U, 1U};
    LineFollowInput outer = {150U, 150U, 1U, 0U, 1U};
    LineFollowOutput inner_output;
    LineFollowOutput outer_output;

    LineFollowControl_Reset();
    inner_output = LineFollowControl_Update(inner);
    LineFollowControl_Reset();
    outer_output = LineFollowControl_Update(outer);

    TEST_ASSERT_TRUE(outer_output.pid_output > inner_output.pid_output);
    TEST_ASSERT_TRUE(outer_output.pid_output <= 1500);
    TEST_ASSERT_TRUE(outer_output.left_target > outer_output.right_target);
}

static void test_large_position_error_reduces_base_speed(void)
{
    LineFollowInput centered = {3500U, 3500U, 0U, 0U, 1U};
    LineFollowInput input = {150U, 150U, 1U, 0U, 1U};
    LineFollowOutput output;
    uint8_t index;

    LineFollowControl_Reset();
    for (index = 0U; index < 40U; index++)
    {
        (void)LineFollowControl_Update(centered);
    }
    output = LineFollowControl_Update(input);

    TEST_ASSERT_EQ_INT(2 * CAR_LINE_CORNER_BASE_SPEED,
                       output.left_target + output.right_target);
}

static void test_line_loss_keeps_current_speed_and_last_direction(void)
{
    LineFollowInput centered = {3500U, 3500U, 0U, 0U, 1U};
    LineFollowInput visible_right = {150U, 3500U, 0U, 0U, 1U};
    LineFollowInput lost = {150U, 150U, 0U, 0U, 1U};
    LineFollowOutput output;
    uint8_t index;

    LineFollowControl_Reset();
    for (index = 0U; index < 50U; index++)
    {
        (void)LineFollowControl_Update(centered);
    }
    (void)LineFollowControl_Update(visible_right);
    for (index = 0U; index < 20U; index++)
    {
        output = LineFollowControl_Update(lost);
        TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
        TEST_ASSERT_EQ_INT(2 * CAR_LINE_PID_BASE_SPEED,
                           output.left_target + output.right_target);
        TEST_ASSERT_TRUE(output.left_target > output.right_target);
    }
}

static void test_missing_sample_keeps_previous_output(void)
{
    LineFollowInput input = {150U, 3500U, 0U, 0U, 1U};
    LineFollowOutput output;
    LineFollowOutput stale;

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    input.left_adc = 3500U;
    input.right_adc = 150U;
    input.sample_ready = 0U;
    stale = LineFollowControl_Update(input);

    TEST_ASSERT_EQ_INT(output.left_target, stale.left_target);
    TEST_ASSERT_EQ_INT(output.right_target, stale.right_target);
    TEST_ASSERT_EQ_INT(output.pid_output, stale.pid_output);
}

static void test_forced_recovery_stays_slow_until_center_is_stable(void)
{
    LineFollowInput input = {150U, 150U, 0U, 0U, 1U};
    LineFollowOutput output;
    uint8_t index;

    LineFollowControl_Reset();
    LineFollowControl_StartRecovery(LINE_FOLLOW_RESCUE_RIGHT);
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    TEST_ASSERT_EQ_INT(2400, output.left_target);
    TEST_ASSERT_EQ_INT(700, output.right_target);

    input.left_adc = 3500U;
    input.right_adc = 3500U;
    for (index = 0U; index < CAR_LINE_RESCUE_CENTER_CONFIRM_SAMPLES; index++)
    {
        output = LineFollowControl_Update(input);
        TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    }
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
}

static void test_forced_recovery_ignores_stale_samples_and_follows_outer_probe(void)
{
    LineFollowInput input = {3500U, 3500U, 0U, 0U, 0U};
    LineFollowOutput output;
    uint8_t index;

    LineFollowControl_Reset();
    LineFollowControl_StartRecovery(LINE_FOLLOW_RESCUE_RIGHT);
    for (index = 0U; index < 20U; index++)
    {
        output = LineFollowControl_Update(input);
    }
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);

    input.sample_ready = 1U;
    input.outer_left = 1U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    TEST_ASSERT_EQ_INT(700, output.left_target);
    TEST_ASSERT_EQ_INT(2400, output.right_target);
}

static void test_wheel_targets_stay_in_valid_range(void)
{
    LineFollowInput input = {0U, 4095U, 1U, 0U, 1U};
    LineFollowOutput output;

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);

    TEST_ASSERT_TRUE(output.left_target >= 0);
    TEST_ASSERT_TRUE(output.left_target <= CAR_LINE_PID_MAX_SPEED);
    TEST_ASSERT_TRUE(output.right_target >= 0);
    TEST_ASSERT_TRUE(output.right_target <= CAR_LINE_PID_MAX_SPEED);
}

int main(void)
{
    test_centered_inner_sensors_drive_straight();
    test_right_inner_black_turns_vehicle_right();
    test_outer_probe_has_more_authority_than_inner_probe();
    test_large_position_error_reduces_base_speed();
    test_line_loss_keeps_current_speed_and_last_direction();
    test_missing_sample_keeps_previous_output();
    test_forced_recovery_stays_slow_until_center_is_stable();
    test_forced_recovery_ignores_stale_samples_and_follows_outer_probe();
    test_wheel_targets_stay_in_valid_range();
    return test_failures;
}
