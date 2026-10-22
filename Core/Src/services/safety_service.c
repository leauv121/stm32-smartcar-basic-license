#include "services/safety_service.h"

#include "services/error_manager.h"

static uint8_t stop_latched;

void SafetyService_Init(void)
{
    stop_latched = 0U;
}

MotionRequest SafetyService_Resolve(MotionRequest requested,
                                    uint8_t user_stop,
                                    uint8_t mission_complete)
{
    if ((ErrorManager_HasFatal() != 0U) ||
        (user_stop != 0U) ||
        (mission_complete != 0U))
    {
        stop_latched = 1U;
    }

    if (stop_latched != 0U)
    {
        return MotionRequest_Stop();
    }

    return requested;
}

void SafetyService_Acknowledge(void)
{
    /* 严重故障仍然活动时，按键确认不能绕过安全停车。 */
    if (ErrorManager_HasFatal() == 0U)
    {
        stop_latched = 0U;
    }
}

uint8_t SafetyService_IsStopLatched(void)
{
    return stop_latched;
}
