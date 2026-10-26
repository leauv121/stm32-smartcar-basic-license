#include "control/line_follow_control.h"
#include "config/car_parameters.h"
#include "test_assert.h"

static void test_centered_line_uses_equal_wheel_targets(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2800U, 2800U, 0U, 0U, 1U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);

    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
    TEST_ASSERT_EQ_INT(output.left_target, output.right_target);
    TEST_ASSERT_TRUE(output.left_target > 0);
}

static void test_outer_sensors_immediately_request_baseline_rescue(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2000U, 1U, 0U, 1U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    TEST_ASSERT_EQ_INT(4500, output.left_target);
    TEST_ASSERT_EQ_INT(1400, output.right_target);

    LineFollowControl_Reset();
    input.outer_right = 0U;
    input.outer_left = 1U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    TEST_ASSERT_EQ_INT(1400, output.left_target);
    TEST_ASSERT_EQ_INT(4500, output.right_target);
}

static void test_pid_uses_working_project_derivative_filter(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2200U, 0U, 0U, 1U};

    LineFollowControl_Reset();
    LineFollowControl_Prime(2000U, 2000U);
    output = LineFollowControl_Update(input);

    /* P=200，滤波后的微分误差=100，Kd=0.5。 */
    TEST_ASSERT_EQ_INT(250, output.pid_output);
}

static void test_baseline_speed_ramps_to_3700(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2800U, 2800U, 0U, 0U, 1U};
    uint8_t index;

    LineFollowControl_Reset();
    for (index = 0U; index < 40U; index++)
    {
        output = LineFollowControl_Update(input);
    }

    TEST_ASSERT_EQ_INT(3700, output.left_target);
    TEST_ASSERT_EQ_INT(3700, output.right_target);
}

static void test_return_from_rescue_resynchronizes_pid_history(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2000U, 1U, 0U, 1U};
    uint8_t index;

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);

    input.left_adc = 2600U;
    input.right_adc = 3000U;
    input.outer_right = 0U;
    for (index = 0U; index < CAR_LINE_RESCUE_CENTER_CONFIRM_SAMPLES; index++)
    {
        output = LineFollowControl_Update(input);
        TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    }
    output = LineFollowControl_Update(input);

    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
    TEST_ASSERT_EQ_INT(400, output.pid_output);
}

static void test_ordinary_rescue_preserves_ramped_base_speed(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2800U, 2800U, 0U, 0U, 1U};
    uint8_t index;

    LineFollowControl_Reset();
    for (index = 0U; index < 40U; index++)
    {
        output = LineFollowControl_Update(input);
    }
    TEST_ASSERT_EQ_INT(3700, output.left_target);

    input.outer_right = 1U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);

    input.outer_right = 0U;
    for (index = 0U; index < CAR_LINE_RESCUE_CENTER_CONFIRM_SAMPLES; index++)
    {
        output = LineFollowControl_Update(input);
        TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    }
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
    TEST_ASSERT_EQ_INT(3700, output.left_target);
    TEST_ASSERT_EQ_INT(3700, output.right_target);
}

static void test_left_rescue_survives_outer_sensor_dropout_until_centered(void)
{
    LineFollowOutput output;
    LineFollowInput input = {1800U, 1800U, 0U, 1U, 1U};
    uint8_t index;

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    TEST_ASSERT_EQ_INT(1400, output.left_target);
    TEST_ASSERT_EQ_INT(4500, output.right_target);

    /* 外侧探头短暂变白，而中间探头还没有找回黑线。 */
    input.outer_left = 0U;
    input.left_adc = 500U;
    input.right_adc = 700U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    TEST_ASSERT_EQ_INT(1400, output.left_target);
    TEST_ASSERT_EQ_INT(4500, output.right_target);

    input.left_adc = 2800U;
    input.right_adc = 2800U;
    for (index = 0U; index + 1U < CAR_LINE_RESCUE_CENTER_CONFIRM_SAMPLES; index++)
    {
        output = LineFollowControl_Update(input);
        TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    }
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
}

static void test_rescue_does_not_turn_forever_after_line_dropout(void)
{
    LineFollowOutput output;
    LineFollowInput input = {1800U, 1800U, 0U, 1U, 1U};
    uint8_t index;

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);

    input.outer_left = 0U;
    input.left_adc = 500U;
    input.right_adc = 700U;
    for (index = 0U; index < CAR_LINE_RESCUE_RELEASE_GRACE_SAMPLES; index++)
    {
        output = LineFollowControl_Update(input);
        TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    }
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
}

static void test_low_absolute_adc_does_not_trigger_normal_recovery(void)
{
    LineFollowOutput output;
    LineFollowInput input = {200U, 200U, 0U, 0U, 1U};
    uint8_t index;

    LineFollowControl_Reset();
    for (index = 0U; index < 20U; index++)
    {
        output = LineFollowControl_Update(input);
        TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
        TEST_ASSERT_EQ_INT(output.left_target, output.right_target);
    }
}

static void test_missing_sample_keeps_previous_output(void)
{
    LineFollowOutput output;
    LineFollowOutput stale;
    LineFollowInput input = {2000U, 2200U, 0U, 0U, 1U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    input.left_adc = 3000U;
    input.right_adc = 1000U;
    input.sample_ready = 0U;
    stale = LineFollowControl_Update(input);

    TEST_ASSERT_EQ_INT(output.left_target, stale.left_target);
    TEST_ASSERT_EQ_INT(output.right_target, stale.right_target);
    TEST_ASSERT_EQ_INT(output.pid_output, stale.pid_output);
}

static void test_forced_recovery_stays_slow_until_center_is_stable(void)
{
    LineFollowOutput output;
    LineFollowInput input = {200U, 300U, 0U, 0U, 1U};
    uint8_t index;

    LineFollowControl_Reset();
    LineFollowControl_StartRecovery(LINE_FOLLOW_RESCUE_RIGHT);
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    TEST_ASSERT_EQ_INT(2400, output.left_target);
    TEST_ASSERT_EQ_INT(700, output.right_target);

    input.left_adc = 2800U;
    input.right_adc = 2800U;
    for (index = 0U; index < CAR_LINE_RESCUE_CENTER_CONFIRM_SAMPLES; index++)
    {
        output = LineFollowControl_Update(input);
        TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    }
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
}

static void test_forced_recovery_ignores_stale_samples_and_keeps_searching(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2800U, 2800U, 0U, 0U, 0U};
    uint16_t index;

    LineFollowControl_Reset();
    LineFollowControl_StartRecovery(LINE_FOLLOW_RESCUE_RIGHT);

    for (index = 0U; index < 20U; index++)
    {
        output = LineFollowControl_Update(input);
    }
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);

    input.left_adc = 200U;
    input.right_adc = 300U;
    input.sample_ready = 1U;
    for (index = 0U; index < 400U; index++)
    {
        output = LineFollowControl_Update(input);
    }
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    TEST_ASSERT_EQ_INT(2400, output.left_target);
    TEST_ASSERT_EQ_INT(700, output.right_target);

    input.outer_left = 1U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    TEST_ASSERT_EQ_INT(700, output.left_target);
    TEST_ASSERT_EQ_INT(2400, output.right_target);
}

static void test_pid_wheel_targets_stay_in_valid_range(void)
{
    LineFollowOutput output;
    LineFollowInput input = {0U, 4095U, 0U, 0U, 1U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);

    TEST_ASSERT_TRUE(output.left_target >= 0);
    TEST_ASSERT_TRUE(output.left_target <= 6500);
    TEST_ASSERT_TRUE(output.right_target >= 0);
    TEST_ASSERT_TRUE(output.right_target <= 6500);
}

int main(void)
{
    test_centered_line_uses_equal_wheel_targets();
    test_outer_sensors_immediately_request_baseline_rescue();
    test_pid_uses_working_project_derivative_filter();
    test_baseline_speed_ramps_to_3700();
    test_return_from_rescue_resynchronizes_pid_history();
    test_ordinary_rescue_preserves_ramped_base_speed();
    test_left_rescue_survives_outer_sensor_dropout_until_centered();
    test_rescue_does_not_turn_forever_after_line_dropout();
    test_low_absolute_adc_does_not_trigger_normal_recovery();
    test_missing_sample_keeps_previous_output();
    test_forced_recovery_stays_slow_until_center_is_stable();
    test_forced_recovery_ignores_stale_samples_and_keeps_searching();
    test_pid_wheel_targets_stay_in_valid_range();
    return test_failures;
}
