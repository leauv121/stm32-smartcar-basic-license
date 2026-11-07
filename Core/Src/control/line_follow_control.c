#include "control/line_follow_control.h"

#include "config/car_parameters.h"

static float last_error;
static float integral;
static float filtered_derivative;
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

static float clamp_float(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static float inner_line_level(uint16_t adc)
{
    const float span = (float)(CAR_LINE_SENSOR_BLACK_ADC -
                               CAR_LINE_SENSOR_WHITE_ADC);
    float level = ((float)adc - (float)CAR_LINE_SENSOR_WHITE_ADC) *
                  CAR_LINE_SENSOR_LEVEL_MAX / span;
    return clamp_float(level, 0.0f, CAR_LINE_SENSOR_LEVEL_MAX);
}

/*
 * 参考工程的四探头位置模型：
 * S1 右外 +3，S2 右内 +1，S3 左内 -1，S4 左外 -3。
 * 返回值只表示是否至少有一个探头看到黑线。
 */
static uint8_t line_position(LineFollowInput input, float *error)
{
    const float weights[4] = {3.0f, 1.0f, -1.0f, -3.0f};
    float levels[4];
    float sum = 0.0f;
    float weighted = 0.0f;
    uint8_t visible = 0U;
    uint8_t index;

    levels[0] = (input.outer_right != 0U) ?
                CAR_LINE_SENSOR_LEVEL_MAX : 0.0f;
    levels[1] = inner_line_level(input.right_adc);
    levels[2] = inner_line_level(input.left_adc);
    levels[3] = (input.outer_left != 0U) ?
                CAR_LINE_SENSOR_LEVEL_MAX : 0.0f;

    for (index = 0U; index < 4U; index++)
    {
        float level = levels[index];
        if (level >= CAR_LINE_SENSOR_DETECT_LEVEL)
        {
            visible = 1U;
        }
        level = clamp_float(level - CAR_LINE_SENSOR_NOISE_FLOOR,
                            0.0f, CAR_LINE_SENSOR_LEVEL_MAX);
        sum += level;
        weighted += level * weights[index];
    }

    if ((visible != 0U) && (sum > 0.0f))
    {
        *error = weighted / sum;
        return 1U;
    }
    return 0U;
}

/* 掉头后的强制找线仍使用原有的稳定居中判据。 */
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

static void resume_pid(uint8_t restart_speed)
{
    last_error = 0.0f;
    integral = 0.0f;
    filtered_derivative = 0.0f;
    primed = 0U;
    if (restart_speed != 0U)
    {
        base_speed = CAR_LINE_PID_START_SPEED;
    }
    current_mode = LINE_FOLLOW_PID;
    forced_recovery = 0U;
    recenter_count = 0U;
}

void LineFollowControl_Reset(void)
{
    base_speed = CAR_LINE_PID_START_SPEED;
    current_mode = LINE_FOLLOW_PID;
    forced_recovery = 0U;
    recenter_count = 0U;
    last_output = (LineFollowOutput){0, 0, 0, LINE_FOLLOW_PID};
    resume_pid(1U);
}

void LineFollowControl_Prime(uint16_t left_adc, uint16_t right_adc)
{
    LineFollowInput input = {left_adc, right_adc, 0U, 0U, 1U};
    float error = 0.0f;

    resume_pid(1U);
    if (line_position(input, &error) != 0U)
    {
        last_error = error;
        primed = 1U;
    }
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
    const float dt = (float)CAR_CONTROL_INTERVAL_MS / 1000.0f;
    LineFollowOutput output = {0, 0, 0, LINE_FOLLOW_PID};
    float error = 0.0f;
    float raw_derivative;
    float correction;
    float absolute_error;
    int32_t desired_base;
    uint8_t visible;

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
                resume_pid(1U);
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

    visible = line_position(input, &error);
    if (visible == 0U)
    {
        /* 丢线时保持当前车速，并沿用最后一次有效纠偏方向继续行驶。 */
        error = (primed != 0U) ? last_error : 0.0f;
    }

    if (primed == 0U)
    {
        last_error = error;
        primed = 1U;
    }

    integral = clamp_float(integral + error * dt,
                           -CAR_LINE_POSITION_INTEGRAL_LIMIT,
                           CAR_LINE_POSITION_INTEGRAL_LIMIT);
    raw_derivative = (error - last_error) / dt;
    filtered_derivative = 0.65f * filtered_derivative +
                          0.35f * raw_derivative;
    correction = CAR_LINE_POSITION_KP * error +
                 CAR_LINE_POSITION_KI * integral +
                 CAR_LINE_POSITION_KD * filtered_derivative;
    correction = clamp_float(
        correction * CAR_LINE_CORRECTION_PWM_PER_MPS,
        -(float)CAR_LINE_CORRECTION_MAX_PWM,
        (float)CAR_LINE_CORRECTION_MAX_PWM);
    output.pid_output = (int32_t)correction;
    last_error = error;

    absolute_error = (error < 0.0f) ? -error : error;
    if (visible == 0U)
    {
        desired_base = base_speed;
    }
    else if (absolute_error > CAR_LINE_CORNER_ERROR_THRESHOLD)
    {
        desired_base = CAR_LINE_CORNER_BASE_SPEED;
    }
    else
    {
        desired_base = CAR_LINE_PID_BASE_SPEED;
    }

    if (base_speed > desired_base)
    {
        base_speed = desired_base;
    }
    else
    {
        base_speed = clamp_value(base_speed + CAR_LINE_PID_SOFT_START_STEP,
                                 CAR_LINE_PID_START_SPEED,
                                 desired_base);
    }

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
