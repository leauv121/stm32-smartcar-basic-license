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

/* 绝对阈值只服务于掉头后的强制找线，不参与普通循迹。 */
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

static void resume_pid(uint16_t left_adc,
                       uint16_t right_adc,
                       uint8_t restart_speed)
{
    last_error = line_error(left_adc, right_adc);
    integral = 0;
    filtered_derivative = 0;
    if (restart_speed != 0U)
    {
        base_speed = CAR_LINE_PID_START_SPEED;
    }
    primed = 1U;
    current_mode = LINE_FOLLOW_PID;
    forced_recovery = 0U;
    recenter_count = 0U;
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
    last_output = (LineFollowOutput){0, 0, 0, LINE_FOLLOW_PID};
}

void LineFollowControl_Prime(uint16_t left_adc, uint16_t right_adc)
{
    resume_pid(left_adc, right_adc, 1U);
    last_output = (LineFollowOutput){0, 0, 0, LINE_FOLLOW_PID};
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
    last_output = recovery_output(direction);
}

LineFollowOutput LineFollowControl_Update(LineFollowInput input)
{
    LineFollowOutput output = {0, 0, 0, LINE_FOLLOW_PID};
    int32_t error;
    int32_t raw_derivative;

    if (input.sample_ready == 0U)
    {
        return last_output;
    }

    if (current_mode == LINE_FOLLOW_FAULT)
    {
        return last_output;
    }

    if (forced_recovery != 0U)
    {
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
            if (recenter_count < CAR_LINE_RESCUE_CENTER_CONFIRM_SAMPLES)
            {
                recenter_count++;
            }
            else
            {
                resume_pid(input.left_adc, input.right_adc, 1U);
            }
        }
        else
        {
            recenter_count = 0U;
        }

        if (forced_recovery != 0U)
        {
            last_output = recovery_output(current_mode);
            return last_output;
        }
    }

    /* 外侧探头请求固定差速救线；释放前要求中间探头连续居中，
       避免弯道中的一次采样抖动把救线状态误切回 PID。 */
    if ((input.outer_right != 0U) && (input.outer_left == 0U))
    {
        current_mode = LINE_FOLLOW_RESCUE_RIGHT;
        forced_recovery = 0U;
        recenter_count = 0U;
    }
    else if ((input.outer_left != 0U) && (input.outer_right == 0U))
    {
        current_mode = LINE_FOLLOW_RESCUE_LEFT;
        forced_recovery = 0U;
        recenter_count = 0U;
    }

    if (current_mode != LINE_FOLLOW_PID)
    {
        if ((input.outer_right != 0U) || (input.outer_left != 0U))
        {
            recenter_count = 0U;
        }
        else if (line_is_centered(input) != 0U)
        {
            if (recenter_count < CAR_LINE_RESCUE_CENTER_CONFIRM_SAMPLES)
            {
                recenter_count++;
            }
            else
            {
                resume_pid(input.left_adc, input.right_adc, 0U);
            }
        }
        else
        {
            if (recenter_count < CAR_LINE_RESCUE_RELEASE_GRACE_SAMPLES)
            {
                recenter_count++;
            }
            else
            {
                resume_pid(input.left_adc, input.right_adc, 0U);
            }
        }
    }

    if (current_mode != LINE_FOLLOW_PID)
    {
        last_output = recovery_output(current_mode);
        return last_output;
    }

    if (current_mode != LINE_FOLLOW_PID)
    {
        /* 普通救线释放后同步当前误差，并保留已经爬升的基础速度。 */
        resume_pid(input.left_adc, input.right_adc, 0U);
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
    output.mode = LINE_FOLLOW_PID;
    current_mode = LINE_FOLLOW_PID;
    last_output = output;
    return last_output;
}
