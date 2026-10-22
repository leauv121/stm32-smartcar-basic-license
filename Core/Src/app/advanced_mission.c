#include "app/advanced_mission.h"
#include "config/car_parameters.h"

static AdvancedMissionState state;
static AppMode active_mode;
static uint8_t route_index;
static uint8_t lap_count;

static const RouteEvent circle_route[] = {ROUTE_POINT_B, ROUTE_POINT_C, ROUTE_POINT_D, ROUTE_POINT_A};
static const RouteEvent eight_route[] = {ROUTE_POINT_C, ROUTE_POINT_B, ROUTE_POINT_D, ROUTE_POINT_A};

static const RouteEvent *current_route(void)
{
    if (active_mode == APP_MODE_CIRCLE) return circle_route;
    if (active_mode == APP_MODE_CHALLENGE && ((lap_count % 2U) == 0U)) return circle_route;
    return eight_route;
}

void AdvancedMission_Init(void) { state = ADVANCED_WAIT_START; active_mode = APP_MODE_NONE; route_index = 0U; lap_count = 0U; }
void AdvancedMission_Start(AppMode mode, uint32_t now_ms) { (void)now_ms; active_mode = mode; route_index = 0U; lap_count = 0U; state = ADVANCED_RUNNING; }
void AdvancedMission_Stop(void) { state = ADVANCED_WAIT_START; }
AdvancedMissionState AdvancedMission_GetState(void) { return state; }
uint8_t AdvancedMission_GetLapCount(void) { return lap_count; }

MissionOutput AdvancedMission_Update(const AdvancedMissionInput *input, uint32_t now_ms)
{
    MissionOutput output = {MotionRequest_Stop(), INDICATOR_OFF, 0U, 0U};
    (void)now_ms;
    if (state == ADVANCED_RUNNING)
    {
        output.motion.mode = MOTION_DRIVE;
        output.motion.left_target = CAR_LINE_PID_BASE_SPEED;
        output.motion.right_target = CAR_LINE_PID_BASE_SPEED;
        if ((input != 0) && (input->fatal_error != 0U)) state = ADVANCED_FAULT;
        else if ((input != 0) && (input->route_event == current_route()[route_index]))
        {
            route_index++;
            output.point_reached = 1U;
            output.indicator = INDICATOR_HAZARD;
            if (route_index >= 4U)
            {
                route_index = 0U;
                lap_count++;
                if ((active_mode == APP_MODE_CIRCLE) || (active_mode == APP_MODE_EIGHT) ||
                    ((active_mode == APP_MODE_STABLE) && (lap_count >= 3U))) state = ADVANCED_COMPLETE;
            }
        }
    }
    if (state == ADVANCED_COMPLETE) { output = (MissionOutput){MotionRequest_Stop(), INDICATOR_OFF, 1U, 0U}; }
    if (state == ADVANCED_FAULT) output = (MissionOutput){MotionRequest_Stop(), INDICATOR_HAZARD, 0U, 0U};
    return output;
}
