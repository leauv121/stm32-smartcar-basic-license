#include "control/line_follow_control.h"
#include "config/car_parameters.h"
#include "test_assert.h"

static void test_centered_line_drives_both_wheels_equally(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2000U, 0U, 0U, 1U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(output.left_target, output.right_target);
    TEST_ASSERT_TRUE(output.left_target > 0);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
}

static void test_outer_sensors_request_rescue_turns(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2000U, 1U, 0U, 1U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    TEST_ASSERT_EQ_INT(3600, output.left_target);
    TEST_ASSERT_EQ_INT(1400, output.right_target);

    input.outer_right = 0U;
    input.outer_left = 1U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_LEFT, output.mode);
    TEST_ASSERT_EQ_INT(1400, output.left_target);
    TEST_ASSERT_EQ_INT(3600, output.right_target);
}

static void test_stable_center_accelerates_and_sustained_deviation_slows(void)
{
    LineFollowOutput output;
    LineFollowInput centered = {2800U, 2800U, 0U, 0U, 1U};
    LineFollowInput left_lost = {200U, 3000U, 0U, 0U, 1U};
    int32_t fast_average;
    int32_t slow_average;
    uint8_t index;

    LineFollowControl_Reset();
    for (index = 0U; index < 60U; index++)
    {
        output = LineFollowControl_Update(centered);
    }
    fast_average = ((int32_t)output.left_target + output.right_target) / 2;
    TEST_ASSERT_EQ_INT(CAR_LINE_CENTER_FAST_SPEED, fast_average);

    output = LineFollowControl_Update(left_lost);
    TEST_ASSERT_EQ_INT(CAR_LINE_CENTER_FAST_SPEED - CAR_LINE_SPEED_DECEL_STEP,
                       ((int32_t)output.left_target + output.right_target) / 2);
    left_lost.sample_ready = 0U;
    {
        LineFollowOutput stale = LineFollowControl_Update(left_lost);
        TEST_ASSERT_EQ_INT(output.left_target, stale.left_target);
        TEST_ASSERT_EQ_INT(output.right_target, stale.right_target);
    }
    left_lost.sample_ready = 1U;
    output = LineFollowControl_Update(left_lost);
    output = LineFollowControl_Update(left_lost);
    slow_average = ((int32_t)output.left_target + output.right_target) / 2;
    TEST_ASSERT_TRUE(slow_average < fast_average);
    TEST_ASSERT_TRUE(output.pid_output <= CAR_LINE_CAUTION_OUTPUT_LIMIT);

    output = LineFollowControl_Update(left_lost);
    output = LineFollowControl_Update(left_lost);
    output = LineFollowControl_Update(left_lost);
    TEST_ASSERT_EQ_INT(CAR_LINE_CAUTION_SPEED,
                       ((int32_t)output.left_target + output.right_target) / 2);

    left_lost.left_adc = 3000U;
    left_lost.right_adc = 200U;
    output = LineFollowControl_Update(left_lost);
    TEST_ASSERT_TRUE(output.pid_output >= -CAR_LINE_CAUTION_OUTPUT_LIMIT);
}

static void test_complete_line_loss_enters_slow_search_in_last_error_direction(void)
{
    LineFollowOutput output;
    LineFollowInput drifting_right = {1800U, 2600U, 0U, 0U, 1U};
    LineFollowInput lost = {200U, 200U, 0U, 0U, 1U};
    uint8_t index;

    LineFollowControl_Reset();
    (void)LineFollowControl_Update(drifting_right);
    for (index = 0U; index < CAR_LINE_LOST_SEARCH_SAMPLES; index++)
    {
        output = LineFollowControl_Update(lost);
    }
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    TEST_ASSERT_EQ_INT(CAR_LINE_REACQUIRE_OUTER_SPEED, output.left_target);
    TEST_ASSERT_EQ_INT(CAR_LINE_REACQUIRE_INNER_SPEED, output.right_target);
}

static void test_lost_line_noise_does_not_reverse_last_valid_search_direction(void)
{
    LineFollowOutput output;
    LineFollowInput valid_right_error = {1800U, 2600U, 0U, 0U, 1U};
    LineFollowInput noisy_lost = {500U, 100U, 0U, 0U, 1U};
    uint8_t index;

    LineFollowControl_Reset();
    (void)LineFollowControl_Update(valid_right_error);
    for (index = 0U; index < CAR_LINE_LOST_SEARCH_SAMPLES; index++)
    {
        output = LineFollowControl_Update(noisy_lost);
    }
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    TEST_ASSERT_TRUE(output.left_target > output.right_target);
}

static void test_pid_derivative_uses_low_pass_filter(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2200U, 0U, 0U, 1U};

    LineFollowControl_Reset();
    LineFollowControl_Prime(2000U, 2000U);
    output = LineFollowControl_Update(input);

    /* 原车参数下：P=200，滤波后的D误差=100，Kd=0.5。 */
    TEST_ASSERT_EQ_INT(250, output.pid_output);
}

static void test_returning_from_rescue_resynchronizes_pid_history(void)
{
    LineFollowOutput output;
    LineFollowInput input = {2000U, 2000U, 1U, 0U, 1U};

    LineFollowControl_Reset();
    LineFollowControl_Prime(2000U, 2000U);
    (void)LineFollowControl_Update(input);
    (void)LineFollowControl_Update(input);

    input.left_adc = 2600U;
    input.right_adc = 3000U;
    input.outer_right = 0U;
    output = LineFollowControl_Update(input);

    /* 外侧刚释放但中间尚未稳定时，不能立即退出救线。 */
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);

    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    output = LineFollowControl_Update(input);

    /* 连续三个新样本确认中间重新捕获黑线后，才同步历史并返回 PID。 */
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
    TEST_ASSERT_EQ_INT(400, output.pid_output);
}

static void test_forced_reacquire_holds_slow_turn_until_center_is_stable(void)
{
    LineFollowOutput output;
    LineFollowInput input = {200U, 300U, 0U, 0U, 1U};

    LineFollowControl_Reset();
    LineFollowControl_StartRecovery(LINE_FOLLOW_RESCUE_RIGHT);
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    TEST_ASSERT_EQ_INT(2400, output.left_target);
    TEST_ASSERT_EQ_INT(700, output.right_target);

    /* 没有新循迹样本时必须保持上次输出，不能累计回中确认次数。 */
    input.left_adc = 2800U;
    input.right_adc = 2800U;
    input.sample_ready = 0U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);

    input.sample_ready = 1U;
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);
    output = LineFollowControl_Update(input);
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_PID, output.mode);
    TEST_ASSERT_TRUE(output.left_target < 2500);
    TEST_ASSERT_TRUE(output.right_target < 2500);
}

static void test_pid_output_is_clamped_to_safe_range(void)
{
    LineFollowOutput output;
    LineFollowInput input = {0U, 4095U, 0U, 0U, 1U};

    LineFollowControl_Reset();
    output = LineFollowControl_Update(input);
    TEST_ASSERT_TRUE(output.left_target <= 6500);
    TEST_ASSERT_TRUE(output.right_target >= 0);
}

static void test_recovery_timeout_stops_instead_of_turning_forever(void)
{
    LineFollowOutput output;
    LineFollowInput input = {200U, 300U, 0U, 0U, 0U};
    uint16_t index;

    LineFollowControl_Reset();
    LineFollowControl_StartRecovery(LINE_FOLLOW_RESCUE_RIGHT);

    /* 没有新样本时不消耗恢复预算。 */
    for (index = 0U; index < 20U; index++)
    {
        output = LineFollowControl_Update(input);
    }
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_RESCUE_RIGHT, output.mode);

    input.sample_ready = 1U;
    for (index = 0U; index < CAR_LINE_RECOVERY_MAX_SAMPLES; index++)
    {
        output = LineFollowControl_Update(input);
    }
    TEST_ASSERT_EQ_INT(LINE_FOLLOW_FAULT, output.mode);
    TEST_ASSERT_EQ_INT(0, output.left_target);
    TEST_ASSERT_EQ_INT(0, output.right_target);
}

int main(void)
{
    test_centered_line_drives_both_wheels_equally();
    test_outer_sensors_request_rescue_turns();
    test_stable_center_accelerates_and_sustained_deviation_slows();
    test_complete_line_loss_enters_slow_search_in_last_error_direction();
    test_lost_line_noise_does_not_reverse_last_valid_search_direction();
    test_pid_derivative_uses_low_pass_filter();
    test_returning_from_rescue_resynchronizes_pid_history();
    test_forced_reacquire_holds_slow_turn_until_center_is_stable();
    test_pid_output_is_clamped_to_safe_range();
    test_recovery_timeout_stops_instead_of_turning_forever();
    return test_failures;
}
