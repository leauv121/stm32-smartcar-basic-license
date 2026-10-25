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
static uint16_t recovery_sample_count;
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
    recovery_sample_count = 0U;
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
    recovery_sample_count = 0U;
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
    recovery_sample_count = 0U;
    last_output = recovery_output(direction);
}

LineFollowOutput LineFollowControl_Update(LineFollowInput input)
{
    LineFollowOutput output = {0, 0, 0, LINE_FOLLOW_PID};
    int32_t error;
    int32_t raw_derivative;
    LineFollowMode previous_mode;

    if (input.sample_ready == 0U)
    {
        return last_output;
    }

    previous_mode = current_mode;

    if (current_mode != LINE_FOLLOW_PID)
    {
        if (current_mode == LINE_FOLLOW_FAULT)
        {
            return last_output;
        }
        if ((input.outer_right != 0U) && (input.outer_left == 0U))
        {
            current_mode = LINE_FOLLOW_RESCUE_RIGHT;
            recenter_count = 0U;
        }
        else if ((input.outer_left != 0U) && (input.outer_right == 0U))
        {
            current_mode = LINE_FOLLOW_RESCUE_LEFT;
            recenter_count = 0U;
        }
        else if (line_is_centered(input) != 0U)
        {
            if (recenter_count < CAR_LINE_RECENTER_CONFIRM_SAMPLES)
            {
                recenter_count++;
            }
        }
        else
        {
            recenter_count = 0U;
        }

        if (recenter_count < CAR_LINE_RECENTER_CONFIRM_SAMPLES)
        {
            if (recovery_sample_count < CAR_LINE_RECOVERY_MAX_SAMPLES)
            {
                recovery_sample_count++;
            }
            if (recovery_sample_count >= CAR_LINE_RECOVERY_MAX_SAMPLES)
            {
                current_mode = LINE_FOLLOW_FAULT;
                last_output = (LineFollowOutput){0, 0, 0, LINE_FOLLOW_FAULT};
                return last_output;
            }
            last_output = recovery_output(current_mode);
            return last_output;
        }

        /* 中间探头稳定重新捕获黑线后，以低速重新预置 PID。 */
        current_mode = LINE_FOLLOW_PID;
        forced_recovery = 0U;
        recenter_count = 0U;
        recovery_sample_count = 0U;
        last_error = line_error(input.left_adc, input.right_adc);
        integral = 0;
        filtered_derivative = 0;
        base_speed = CAR_LINE_PID_START_SPEED;
        primed = 1U;
    }

    if ((input.outer_right != 0U) && (input.outer_left == 0U))
    {
        current_mode = LINE_FOLLOW_RESCUE_RIGHT;
        forced_recovery = 0U;
        recenter_count = 0U;
        recovery_sample_count = 1U;
        last_output = recovery_output(current_mode);
        return last_output;
    }
    if ((input.outer_left != 0U) && (input.outer_right == 0U))
    {
        current_mode = LINE_FOLLOW_RESCUE_LEFT;
        forced_recovery = 0U;
        recenter_count = 0U;
        recovery_sample_count = 1U;
        last_output = recovery_output(current_mode);
        return last_output;
    }

    current_mode = LINE_FOLLOW_PID;
    error = line_error(input.left_adc, input.right_adc);
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
    integral = clamp_value(integral + error,
                           -CAR_LINE_PID_INTEGRAL_LIMIT,
                           CAR_LINE_PID_INTEGRAL_LIMIT);

    /* 新旧微分各占一半，抑制 ADC 小波动被 Kd 放大。 */
    raw_derivative = error - last_error;
    filtered_derivative = (filtered_derivative + raw_derivative) / 2;
    output.pid_output = (int32_t)(CAR_LINE_PID_KP * (float)error +
                                  CAR_LINE_PID_KI * (float)integral +
                                  CAR_LINE_PID_KD * (float)filtered_derivative);
    last_error = error;

    base_speed = clamp_value(base_speed + CAR_LINE_PID_SOFT_START_STEP,
                             CAR_LINE_PID_START_SPEED,
                             CAR_LINE_PID_BASE_SPEED);
    output.left_target = (int16_t)clamp_value(base_speed + output.pid_output,
                                               0,
                                               CAR_LINE_PID_MAX_SPEED);
    output.right_target = (int16_t)clamp_value(base_speed - output.pid_output,
                                                0,
                                                CAR_LINE_PID_MAX_SPEED);
    last_output = output;
    return last_output;
}
