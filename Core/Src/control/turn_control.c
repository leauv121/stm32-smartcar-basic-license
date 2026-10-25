#include "control/turn_control.h"

#define TURN_MAX_DURATION_MS 5000U

static TurnResult turn_state;
static TurnDirection turn_direction;
static float target_angle;
static float progress_angle;
static float last_yaw;
static uint32_t started_ms;

void TurnControl_Reset(void)
{
    turn_state = TURN_IDLE;
    target_angle = 0.0f;
    progress_angle = 0.0f;
    last_yaw = 0.0f;
    started_ms = 0U;
}

void TurnControl_Start(float current_yaw,
                       float target_deg,
                       TurnDirection direction,
                       uint32_t now_ms)
{
    turn_state = TURN_RUNNING;
    turn_direction = direction;
    target_angle = target_deg;
    progress_angle = 0.0f;
    last_yaw = current_yaw;
    started_ms = now_ms;
}

TurnResult TurnControl_Update(float current_yaw, uint8_t yaw_valid, uint32_t now_ms)
{
    float delta;
    float directed_delta;

    if (turn_state != TURN_RUNNING) return turn_state;
    if (yaw_valid == 0U)
    {
        turn_state = TURN_SENSOR_ERROR;
        return turn_state;
    }
    if ((uint32_t)(now_ms - started_ms) > TURN_MAX_DURATION_MS)
    {
        turn_state = TURN_TIMEOUT;
        return turn_state;
    }

    delta = current_yaw - last_yaw;
    if (delta > 180.0f) delta -= 360.0f;
    else if (delta < -180.0f) delta += 360.0f;
    last_yaw = current_yaw;

    directed_delta = delta * (float)turn_direction;
    /* 使用净转角累计：反向回摆必须扣除，避免不足 180 度就提前完成。 */
    progress_angle += directed_delta;
    if (progress_angle < 0.0f) progress_angle = 0.0f;
    if (progress_angle >= target_angle) turn_state = TURN_COMPLETE;
    return turn_state;
}

float TurnControl_GetProgress(void)
{
    return progress_angle;
}
