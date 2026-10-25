#include "services/driver_algorithms.h"

float DriverAlgorithms_UltrasonicCm(uint32_t pulse_us)
{
    /* 声速按 340 m/s 计算，往返距离需除以 2。 */
    return (float)pulse_us * 0.017f;
}

float DriverAlgorithms_SpeedRpm(uint32_t revolution_ticks, uint32_t timer_hz)
{
    if ((revolution_ticks == 0U) || (timer_hz == 0U))
    {
        return 0.0f;
    }
    return ((float)timer_hz * 60.0f) / (float)revolution_ticks;
}

uint8_t DriverAlgorithms_IsTimedOut(uint32_t sample_ms,
                                    uint32_t now_ms,
                                    uint32_t timeout_ms)
{
    return (uint8_t)((uint32_t)(now_ms - sample_ms) >= timeout_ms);
}

uint8_t DriverAlgorithms_LineDigitalIsBlack(uint8_t raw_level)
{
    /* 原车外侧 DO 传感器为高电平有效：1 表示检测到黑线。 */
    return (uint8_t)(raw_level != 0U);
}

DriverColor DriverAlgorithms_ConfirmColor(ColorConfirmState *state,
                                          DriverColor sample,
                                          uint8_t required_count)
{
    if ((state == 0) || (sample == DRIVER_COLOR_NONE) || (required_count == 0U))
    {
        if (state != 0)
        {
            state->candidate = DRIVER_COLOR_NONE;
            state->emitted = DRIVER_COLOR_NONE;
            state->consecutive_count = 0U;
        }
        return DRIVER_COLOR_NONE;
    }

    if (sample != state->candidate)
    {
        state->candidate = sample;
        state->consecutive_count = 1U;
    }
    else if (state->consecutive_count < required_count)
    {
        state->consecutive_count++;
    }

    if (state->consecutive_count >= required_count)
    {
        state->emitted = state->candidate;
    }
    /* 返回当前稳定状态而不是单次事件，保证持续刷新传感器时间戳。 */
    return state->emitted;
}

void DriverAlgorithms_KeyInit(KeyDebounceState *state,
                              uint8_t initial_level,
                              uint32_t now_ms)
{
    if (state != 0)
    {
        state->stable_level = initial_level;
        state->candidate_level = initial_level;
        state->candidate_since_ms = now_ms;
    }
}

uint8_t DriverAlgorithms_KeyUpdate(KeyDebounceState *state,
                                   uint8_t raw_level,
                                   uint32_t now_ms,
                                   uint32_t debounce_ms)
{
    if (state == 0)
    {
        return 0U;
    }

    if (raw_level != state->candidate_level)
    {
        state->candidate_level = raw_level;
        state->candidate_since_ms = now_ms;
        return 0U;
    }

    if ((raw_level != state->stable_level) &&
        ((uint32_t)(now_ms - state->candidate_since_ms) >= debounce_ms))
    {
        state->stable_level = raw_level;
        return raw_level;
    }
    return 0U;
}
