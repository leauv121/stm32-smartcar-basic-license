#include "control/line_follow_control.h"

#include "config/car_parameters.h"

static int32_t last_error;
static int32_t integral;
static int32_t filtered_derivative;
static int32_t base_speed;
static uint8_t primed;
static LineFollowMode current_mode;

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

void LineFollowControl_Reset(void)
{
    last_error = 0;
    integral = 0;
    filtered_derivative = 0;
    base_speed = CAR_LINE_PID_START_SPEED;
    primed = 0U;
    current_mode = LINE_FOLLOW_PID;
}

void LineFollowControl_Prime(uint16_t left_adc, uint16_t right_adc)
{
    last_error = line_error(left_adc, right_adc);
    integral = 0;
    filtered_derivative = 0;
    base_speed = CAR_LINE_PID_START_SPEED;
    primed = 1U;
    current_mode = LINE_FOLLOW_PID;
}

LineFollowOutput LineFollowControl_Update(LineFollowInput input)
{
    LineFollowOutput output = {0, 0, 0, LINE_FOLLOW_PID};
    int32_t error;
    int32_t raw_derivative;
    LineFollowMode previous_mode = current_mode;

    if ((input.outer_right != 0U) && (input.outer_left == 0U))
    {
        output.left_target = CAR_LINE_RESCUE_OUTER_SPEED;
        output.right_target = CAR_LINE_RESCUE_INNER_SPEED;
        output.mode = LINE_FOLLOW_RESCUE_RIGHT;
        current_mode = output.mode;
        return output;
    }
    if ((input.outer_left != 0U) && (input.outer_right == 0U))
    {
        output.left_target = CAR_LINE_RESCUE_INNER_SPEED;
        output.right_target = CAR_LINE_RESCUE_OUTER_SPEED;
        output.mode = LINE_FOLLOW_RESCUE_LEFT;
        current_mode = output.mode;
        return output;
    }

    current_mode = LINE_FOLLOW_PID;
    error = line_error(input.left_adc, input.right_adc);
    if (previous_mode != LINE_FOLLOW_PID)
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
    return output;
}
