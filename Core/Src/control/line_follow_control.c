#include "control/line_follow_control.h"

#include "config/car_parameters.h"

static int32_t last_error;
static int32_t integral;
static int32_t filtered_derivative;
static int32_t base_speed;
static uint8_t primed;
static LineFollowMode current_mode;
static uint8_t forced_recovery;
static uint8_t recenter_count;
static uint8_t centered_sample_count;
static uint8_t deviation_sample_count;
static uint8_t lost_sample_count;
static uint8_t outer_right_count;
static uint8_t outer_left_count;
static uint8_t curve_candidate_count;
static uint8_t curve_release_count;
static uint8_t curve_active;
static int32_t curve_direction;
static LineFollowMode last_search_direction;
static LineFollowOutput last_output;

static int32_t clamp_value(int32_t value, int32_t minimum, int32_t maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static int32_t line_error(uint16_t left_adc, uint16_t right_adc)
{
    int32_t error = (int32_t)right_adc - (int32_t)left_adc;
    if ((error <= CAR_LINE_PID_ERROR_DEADBAND) &&
        (error >= -CAR_LINE_PID_ERROR_DEADBAND))
    {
        return 0;
    }
    return error;
}

static uint8_t line_is_centered(LineFollowInput input)
{
    int32_t error = (int32_t)input.right_adc - (int32_t)input.left_adc;
    if (error < 0) error = -error;
    return (uint8_t)((input.outer_right == 0U) &&
                     (input.outer_left == 0U) &&
                     (input.left_adc >= CAR_LINE_BLACK_ADC_MIN) &&
                     (input.right_adc >= CAR_LINE_BLACK_ADC_MIN) &&
                     (error <= CAR_LINE_RECENTER_ERROR_MAX));
}

static LineFollowOutput recovery_output(LineFollowMode mode)
{
    LineFollowOutput output = {0, 0, 0, mode};
    int16_t outer_speed = (forced_recovery != 0U) ?
                          CAR_LINE_REACQUIRE_OUTER_SPEED :
                          CAR_LINE_RESCUE_OUTER_SPEED;
    int16_t inner_speed = (forced_recovery != 0U) ?
                          CAR_LINE_REACQUIRE_INNER_SPEED :
                          CAR_LINE_RESCUE_INNER_SPEED;

    if (mode == LINE_FOLLOW_RESCUE_RIGHT)
    {
        output.left_target = outer_speed;
        output.right_target = inner_speed;
    }
    else
    {
        output.left_target = inner_speed;
        output.right_target = outer_speed;
    }
    return output;
}

static void reset_tracking_confidence(void)
{
    centered_sample_count = 0U;
    deviation_sample_count = 0U;
    lost_sample_count = 0U;
    outer_right_count = 0U;
    outer_left_count = 0U;
    curve_candidate_count = 0U;
    curve_release_count = 0U;
    curve_active = 0U;
    curve_direction = 0;
}

static void increment_u8(uint8_t *value, uint8_t maximum);

static int32_t curve_candidate_direction(uint8_t outer_right,
                                          uint8_t outer_left,
                                          int32_t error)
{
    int32_t magnitude = (error < 0) ? -error : error;

    if (magnitude < CAR_LINE_CURVE_ERROR_MIN)
    {
        return 0;
    }
    /* 外侧传感器方向优先，保持与历史可用版本一致。 */
    if ((outer_right != 0U) && (outer_left == 0U))
    {
        return 1;
    }
    if ((outer_left != 0U) && (outer_right == 0U))
    {
        return -1;
    }
    /* 没有外侧提示时，只有更大的持续误差才视为弯道。 */
    return (magnitude >= (CAR_LINE_CURVE_ERROR_MIN * 2)) ?
           ((error > 0) ? 1 : -1) : 0;
}

static void update_curve_state(LineFollowInput input,
                               int32_t error,
                               uint8_t complete_line_loss)
{
    int32_t candidate_direction = (complete_line_loss == 0U) ?
        curve_candidate_direction(input.outer_right, input.outer_left, error) : 0;

    if (curve_active != 0U)
    {
        if (line_is_centered(input) != 0U)
        {
            increment_u8(&curve_release_count, CAR_LINE_CURVE_RELEASE_SAMPLES);
            if (curve_release_count >= CAR_LINE_CURVE_RELEASE_SAMPLES)
            {
                curve_active = 0U;
                curve_release_count = 0U;
                curve_candidate_count = 0U;
            }
        }
        else
        {
            curve_release_count = 0U;
        }
        return;
    }

    curve_release_count = 0U;
    if (candidate_direction == 0)
    {
        curve_candidate_count = 0U;
        curve_direction = 0;
        return;
    }
    if ((curve_direction != 0) && (candidate_direction != curve_direction))
    {
        curve_candidate_count = 1U;
    }
    else
    {
        increment_u8(&curve_candidate_count, CAR_LINE_CURVE_CONFIRM_SAMPLES);
    }
    curve_direction = candidate_direction;
    if (curve_candidate_count >= CAR_LINE_CURVE_CONFIRM_SAMPLES)
    {
        curve_active = 1U;
    }
}

static void increment_u8(uint8_t *value, uint8_t maximum)
{
    if (*value < maximum) (*value)++;
}

static int32_t approach_speed(int32_t current, int32_t target)
{
    if (current < target)
    {
        current += CAR_LINE_PID_SOFT_START_STEP;
        if (current > target) current = target;
    }
    else if (current > target)
    {
        current -= CAR_LINE_SPEED_DECEL_STEP;
        if (current < target) current = target;
    }
    return current;
}

void LineFollowControl_Reset(void)
{
    last_error = 0;
    integral = 0;
    filtered_derivative = 0;
    base_speed = CAR_LINE_PID_START_SPEED;
    primed = 0U;
    current_mode = LINE_FOLLOW_PID;
    forced_recovery = 0U;
    recenter_count = 0U;
    last_search_direction = LINE_FOLLOW_RESCUE_RIGHT;
    reset_tracking_confidence();
    last_output = (LineFollowOutput){0, 0, 0, LINE_FOLLOW_PID};
}

void LineFollowControl_Prime(uint16_t left_adc, uint16_t right_adc)
{
    last_error = line_error(left_adc, right_adc);
    integral = 0;
    filtered_derivative = 0;
    base_speed = CAR_LINE_PID_START_SPEED;
    primed = 1U;
    current_mode = LINE_FOLLOW_PID;
    forced_recovery = 0U;
    recenter_count = 0U;
    reset_tracking_confidence();
}

void LineFollowControl_StartRecovery(LineFollowMode direction)
{
    if ((direction != LINE_FOLLOW_RESCUE_LEFT) &&
        (direction != LINE_FOLLOW_RESCUE_RIGHT))
    {
        direction = LINE_FOLLOW_RESCUE_RIGHT;
    }
    current_mode = direction;
    forced_recovery = 1U;
    recenter_count = 0U;
    reset_tracking_confidence();
    last_output = recovery_output(direction);
}

LineFollowOutput LineFollowControl_Update(LineFollowInput input)
{
    LineFollowOutput output = {0, 0, 0, LINE_FOLLOW_PID};
    int32_t error;
    int32_t raw_derivative;
    LineFollowMode previous_mode;
    uint8_t complete_line_loss;
    uint8_t center_was_confirmed;

    if (input.sample_ready == 0U)
    {
        return last_output;
    }

    previous_mode = current_mode;
    center_was_confirmed = 0U;

    if (current_mode != LINE_FOLLOW_PID)
    {
        if (current_mode == LINE_FOLLOW_FAULT)
        {
            return last_output;
        }
        if ((input.outer_right != 0U) && (input.outer_left == 0U))
        {
            increment_u8(&outer_right_count, CAR_LINE_OUTER_CONFIRM_SAMPLES);
            outer_left_count = 0U;
            if (outer_right_count >= CAR_LINE_OUTER_CONFIRM_SAMPLES)
            {
                current_mode = LINE_FOLLOW_RESCUE_RIGHT;
            }
            recenter_count = 0U;
        }
        else if ((input.outer_left != 0U) && (input.outer_right == 0U))
        {
            increment_u8(&outer_left_count, CAR_LINE_OUTER_CONFIRM_SAMPLES);
            outer_right_count = 0U;
            if (outer_left_count >= CAR_LINE_OUTER_CONFIRM_SAMPLES)
            {
                current_mode = LINE_FOLLOW_RESCUE_LEFT;
            }
            recenter_count = 0U;
        }
        else if (line_is_centered(input) != 0U)
        {
            outer_right_count = 0U;
            outer_left_count = 0U;
            if (recenter_count < CAR_LINE_RESCUE_CENTER_CONFIRM_SAMPLES)
            {
                recenter_count++;
            }
            else
            {
                /* 第六个有效样本已经保持完成，第七个样本才允许退出。 */
                center_was_confirmed = 1U;
            }
        }
        else
        {
            outer_right_count = 0U;
            outer_left_count = 0U;
            recenter_count = 0U;
        }

        /* 回中后继续保持同方向纠偏一段时间，补足实际弯道角度。 */
        if (center_was_confirmed == 0U)
        {
            /* 持续低速找线，不因位于赛道边缘或找线时间较长而主动停车。 */
            last_output = recovery_output(current_mode);
            return last_output;
        }

        /* 中间探头稳定重新捕获黑线后，以低速重新预置 PID。 */
        current_mode = LINE_FOLLOW_PID;
        forced_recovery = 0U;
        recenter_count = 0U;
        reset_tracking_confidence();
        last_error = line_error(input.left_adc, input.right_adc);
        integral = 0;
        filtered_derivative = 0;
        base_speed = CAR_LINE_PID_START_SPEED;
        primed = 1U;
    }

    current_mode = LINE_FOLLOW_PID;
    error = line_error(input.left_adc, input.right_adc);
    complete_line_loss = (uint8_t)(
        (input.left_adc < CAR_LINE_BLACK_ADC_MIN) &&
        (input.right_adc < CAR_LINE_BLACK_ADC_MIN) &&
        (input.outer_right == 0U) && (input.outer_left == 0U));
    if ((previous_mode != LINE_FOLLOW_PID) && (primed == 0U))
    {
        /* 从急弯救线恢复时同步当前误差，避免微分项发生突跳。 */
        last_error = error;
        integral = 0;
        filtered_derivative = 0;
        primed = 1U;
    }
    else if (primed == 0U)
    {
        last_error = error;
        primed = 1U;
    }

    if ((complete_line_loss == 0U) &&
        (error > CAR_LINE_PID_ERROR_DEADBAND))
    {
        last_search_direction = LINE_FOLLOW_RESCUE_RIGHT;
    }
    else if ((complete_line_loss == 0U) &&
             (error < -CAR_LINE_PID_ERROR_DEADBAND))
    {
        last_search_direction = LINE_FOLLOW_RESCUE_LEFT;
    }

    update_curve_state(input, error, complete_line_loss);

    if (line_is_centered(input) != 0U)
    {
        increment_u8(&centered_sample_count, CAR_LINE_CENTER_FAST_SAMPLES);
        deviation_sample_count = 0U;
        lost_sample_count = 0U;
    }
    else
    {
        centered_sample_count = 0U;
        increment_u8(&deviation_sample_count, CAR_LINE_DEVIATION_SLOW_SAMPLES);
        if (complete_line_loss != 0U)
        {
            increment_u8(&lost_sample_count, CAR_LINE_LOST_SEARCH_SAMPLES);
        }
        else
        {
            lost_sample_count = 0U;
        }
    }

    if (lost_sample_count >= CAR_LINE_LOST_SEARCH_SAMPLES)
    {
        current_mode = last_search_direction;
        forced_recovery = 1U;
        recenter_count = 0U;
        last_output = recovery_output(current_mode);
        return last_output;
    }
    integral = clamp_value(integral + error,
                           -CAR_LINE_PID_INTEGRAL_LIMIT,
                           CAR_LINE_PID_INTEGRAL_LIMIT);

    /* 新旧微分各占一半，抑制 ADC 小波动被 Kd 放大。 */
    raw_derivative = error - last_error;
    filtered_derivative = (filtered_derivative + raw_derivative) / 2;
    output.pid_output = (int32_t)(CAR_LINE_PID_KP * (float)error +
                                  CAR_LINE_PID_KI * (float)integral +
                                  CAR_LINE_PID_KD * (float)filtered_derivative);
    if (curve_active != 0U)
    {
        int32_t magnitude = (output.pid_output < 0) ?
                            -output.pid_output : output.pid_output;
        output.pid_output = (curve_direction < 0) ? -magnitude : magnitude;
        output.pid_output = (output.pid_output * CAR_LINE_CURVE_PID_GAIN_NUM) /
                            CAR_LINE_CURVE_PID_GAIN_DEN;
        output.pid_output = clamp_value(output.pid_output,
                                        -CAR_LINE_CURVE_OUTPUT_LIMIT,
                                        CAR_LINE_CURVE_OUTPUT_LIMIT);
    }
    else
    {
        output.pid_output = clamp_value(output.pid_output,
                                        -CAR_LINE_PID_OUTPUT_LIMIT,
                                        CAR_LINE_PID_OUTPUT_LIMIT);
        if (deviation_sample_count >= CAR_LINE_DEVIATION_SLOW_SAMPLES)
        {
            output.pid_output = clamp_value(output.pid_output,
                                            -CAR_LINE_CAUTION_OUTPUT_LIMIT,
                                            CAR_LINE_CAUTION_OUTPUT_LIMIT);
        }
    }
    last_error = error;

    if (centered_sample_count >= CAR_LINE_CENTER_FAST_SAMPLES)
    {
        base_speed = approach_speed(base_speed, CAR_LINE_CENTER_FAST_SPEED);
    }
    else if ((deviation_sample_count >= CAR_LINE_DEVIATION_SLOW_SAMPLES) ||
             (lost_sample_count != 0U))
    {
        base_speed = approach_speed(base_speed, CAR_LINE_CAUTION_SPEED);
    }
    else
    {
        base_speed = approach_speed(base_speed, CAR_LINE_PID_BASE_SPEED);
    }
    output.left_target = (int16_t)clamp_value(base_speed + output.pid_output,
                                               0,
                                               CAR_LINE_PID_MAX_SPEED);
    output.right_target = (int16_t)clamp_value(base_speed - output.pid_output,
                                                0,
                                                CAR_LINE_PID_MAX_SPEED);
    if (curve_active != 0U)
    {
        if (output.left_target < CAR_LINE_CURVE_MIN_WHEEL_SPEED)
        {
            output.left_target = CAR_LINE_CURVE_MIN_WHEEL_SPEED;
        }
        if (output.right_target < CAR_LINE_CURVE_MIN_WHEEL_SPEED)
        {
            output.right_target = CAR_LINE_CURVE_MIN_WHEEL_SPEED;
        }
    }
    last_output = output;
    return last_output;
}
