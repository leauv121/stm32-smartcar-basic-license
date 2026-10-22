#include "services/indicator_service.h"

static IndicatorPattern current_pattern;
static uint32_t pattern_started_ms;

void IndicatorService_Init(void)
{
    current_pattern = INDICATOR_OFF;
    pattern_started_ms = 0U;
}

void IndicatorService_Start(IndicatorPattern pattern, uint32_t now_ms)
{
    current_pattern = pattern;
    pattern_started_ms = now_ms;
}

IndicatorOutput IndicatorService_Update(uint32_t now_ms)
{
    IndicatorOutput output = {0U, 0U, 0U, 0U, 0U};
    uint32_t elapsed = now_ms - pattern_started_ms;
    uint8_t on_phase;

    switch (current_pattern)
    {
        case INDICATOR_STARTUP_FLASH_3:
            if (elapsed >= 3000U)
            {
                output.completed = 1U;
                current_pattern = INDICATOR_OFF;
                break;
            }
            on_phase = (uint8_t)(((elapsed / 500U) % 2U) == 0U);
            output.left_on = on_phase;
            output.right_on = on_phase;
            output.active = 1U;
            break;

        case INDICATOR_TURN_LEFT:
            on_phase = (uint8_t)(((elapsed / 300U) % 2U) == 0U);
            output.left_on = on_phase;
            output.active = 1U;
            break;

        case INDICATOR_TURN_RIGHT:
            on_phase = (uint8_t)(((elapsed / 300U) % 2U) == 0U);
            output.right_on = on_phase;
            output.active = 1U;
            break;

        case INDICATOR_HAZARD:
            on_phase = (uint8_t)(((elapsed / 500U) % 2U) == 0U);
            output.left_on = on_phase;
            output.right_on = on_phase;
            output.active = 1U;
            break;

        case INDICATOR_OFF:
        default:
            break;
    }

    return output;
}

IndicatorPattern IndicatorService_GetPattern(void)
{
    return current_pattern;
}
