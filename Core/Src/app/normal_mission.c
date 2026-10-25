#include "app/normal_mission.h"
#include "config/car_parameters.h"

static NormalMissionState state;
static uint8_t obstacle_clear_count;
static uint8_t return_obstacle_count;

static void reset_return_detection(void)
{
    obstacle_clear_count = 0U;
    return_obstacle_count = 0U;
}

static MissionOutput output_for_state(void)
{
    MissionOutput output = {MotionRequest_Stop(), INDICATOR_OFF, 0U, 0U};
    if ((state == NORMAL_TRACK_OUT) ||
        (state == NORMAL_RETURN_CLEAR) ||
        (state == NORMAL_RETURN))
    {
        output.motion.mode = MOTION_DRIVE;
        output.motion.left_target = CAR_LINE_PID_BASE_SPEED;
        output.motion.right_target = CAR_LINE_PID_BASE_SPEED;
    }
    else if (state == NORMAL_START_FLASH) output.indicator = INDICATOR_STARTUP_FLASH_3;
    else if ((state == NORMAL_RED_WAIT) || (state == NORMAL_OBSTACLE_HOLD)) output.indicator = INDICATOR_HAZARD;
    else if (state == NORMAL_AVOID_TURN)
    {
        output.motion.mode = MOTION_DRIVE;
        output.motion.left_target = CAR_AVOID_TURN_LEFT_SPEED;
        output.motion.right_target = CAR_AVOID_TURN_RIGHT_SPEED;
        output.indicator = INDICATOR_TURN_RIGHT;
    }
    else if (state == NORMAL_REACQUIRE_LINE)
    {
        output.motion.mode = MOTION_DRIVE;
        output.motion.left_target = CAR_LINE_REACQUIRE_OUTER_SPEED;
        output.motion.right_target = CAR_LINE_REACQUIRE_INNER_SPEED;
        output.indicator = INDICATOR_TURN_RIGHT;
    }
    if (state == NORMAL_COMPLETE) output.complete = 1U;
    return output;
}

void NormalMission_Init(void)
{
    state = NORMAL_WAIT_START;
    reset_return_detection();
}

void NormalMission_Start(uint32_t now_ms)
{
    (void)now_ms;
    state = NORMAL_START_FLASH;
    reset_return_detection();
}

void NormalMission_Stop(void)
{
    state = NORMAL_WAIT_START;
    reset_return_detection();
}
NormalMissionState NormalMission_GetState(void) { return state; }

MissionOutput NormalMission_Update(const NormalMissionInput *input, uint32_t now_ms)
{
    (void)now_ms;
    if (input == 0) return output_for_state();
    if (input->fatal_error != 0U) state = NORMAL_FAULT;
    else if ((state == NORMAL_START_FLASH) && (input->start_flash_done != 0U)) state = NORMAL_TRACK_OUT;
    else if ((state == NORMAL_TRACK_OUT) && (input->red_detected != 0U)) state = NORMAL_RED_WAIT;
    else if ((state == NORMAL_RED_WAIT) && (input->green_detected != 0U)) state = NORMAL_TRACK_OUT;
    else if ((state == NORMAL_TRACK_OUT) && (input->obstacle_detected != 0U)) state = NORMAL_OBSTACLE_HOLD;
    else if ((state == NORMAL_OBSTACLE_HOLD) && (input->hazard_hold_done != 0U)) state = NORMAL_AVOID_TURN;
    else if ((state == NORMAL_AVOID_TURN) && (input->turn_complete != 0U))
    {
        state = NORMAL_REACQUIRE_LINE;
        reset_return_detection();
    }
    else if ((state == NORMAL_REACQUIRE_LINE) && (input->line_reacquired != 0U))
    {
        state = NORMAL_RETURN_CLEAR;
        reset_return_detection();
    }
    else if ((state == NORMAL_RETURN_CLEAR) && (input->obstacle_sample_ready != 0U))
    {
        if (input->obstacle_cleared != 0U)
        {
            if (obstacle_clear_count < CAR_OBSTACLE_CONFIRM_SAMPLES)
            {
                obstacle_clear_count++;
            }
        }
        else
        {
            obstacle_clear_count = 0U;
        }
        if (obstacle_clear_count >= CAR_OBSTACLE_CONFIRM_SAMPLES)
        {
            state = NORMAL_RETURN;
            return_obstacle_count = 0U;
        }
    }
    else if ((state == NORMAL_RETURN) && (input->obstacle_sample_ready != 0U))
    {
        if (input->obstacle_detected != 0U)
        {
            if (return_obstacle_count < CAR_OBSTACLE_CONFIRM_SAMPLES)
            {
                return_obstacle_count++;
            }
        }
        else
        {
            return_obstacle_count = 0U;
        }
        if (return_obstacle_count >= CAR_OBSTACLE_CONFIRM_SAMPLES)
        {
            state = NORMAL_COMPLETE;
        }
    }
    return output_for_state();
}
