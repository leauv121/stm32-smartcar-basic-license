#include "app/route_detector.h"

static RouteEvent candidate_point;
static RouteEvent latched_point;
static uint8_t stable_count;

void RouteDetector_Init(void)
{
    RouteDetector_Reset();
}

void RouteDetector_Reset(void)
{
    candidate_point = ROUTE_EVENT_NONE;
    latched_point = ROUTE_EVENT_NONE;
    stable_count = 0U;
}

RouteEvent RouteDetector_Update(const RouteDetectorInput *input)
{
    if ((input == 0) || (input->candidate_valid == 0U) ||
        (input->candidate == ROUTE_EVENT_NONE))
    {
        candidate_point = ROUTE_EVENT_NONE;
        latched_point = ROUTE_EVENT_NONE;
        stable_count = 0U;
        return ROUTE_EVENT_NONE;
    }

    if (input->candidate != candidate_point)
    {
        candidate_point = input->candidate;
        stable_count = 1U;
        return ROUTE_EVENT_NONE;
    }

    if (stable_count < 2U)
    {
        stable_count++;
    }

    if ((stable_count >= 2U) && (latched_point != candidate_point))
    {
        latched_point = candidate_point;
        return candidate_point;
    }

    return ROUTE_EVENT_NONE;
}
