#include "control/line_follow_control.h"

#include "config/car_parameters.h"

#define RESCUE_OUTER_SPEED 4500
#define RESCUE_INNER_SPEED    0

static int32_t last_error;
static int32_t integral;
static int32_t base_speed;
static uint8_t primed;

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
    base_speed = CAR_LINE_PID_START_SPEED;
    primed = 0U;
}

void LineFollowControl_Prime(uint16_t left_adc, uint16_t right_adc)
{
    last_error = line_error(left_adc, right_adc);
    integral = 0;
    base_speed = CAR_LINE_PID_START_SPEED;
    primed = 1U;
}

LineFollowOutput LineFollowControl_Update(LineFollowInput input)
{
    LineFollowOutput output = {0, 0, 0, LINE_FOLLOW_PID};
    int32_t error;

    if ((input.outer_right != 0U) && (input.outer_left == 0U))
    {
        output.left_target = RESCUE_OUTER_SPEED;
        output.right_target = RESCUE_INNER_SPEED;
        output.mode = LINE_FOLLOW_RESCUE_RIGHT;
        return output;
    }
    if ((input.outer_left != 0U) && (input.outer_right == 0U))
    {
        output.left_target = RESCUE_INNER_SPEED;
        output.right_target = RESCUE_OUTER_SPEED;
        output.mode = LINE_FOLLOW_RESCUE_LEFT;
        return output;
    }

    error = line_error(input.left_adc, input.right_adc);
    if (primed == 0U)
    {
        last_error = error;
        primed = 1U;
    }
    integral = clamp_value(integral + error,
                           -CAR_LINE_PID_INTEGRAL_LIMIT,
                           CAR_LINE_PID_INTEGRAL_LIMIT);
    output.pid_output = (int32_t)(CAR_LINE_PID_KP * (float)error +
                                  CAR_LINE_PID_KI * (float)integral +
                                  CAR_LINE_PID_KD * (float)(error - last_error));
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
