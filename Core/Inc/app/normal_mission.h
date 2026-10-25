#ifndef FINAL_WORK_NORMAL_MISSION_H
#define FINAL_WORK_NORMAL_MISSION_H

#include <stdint.h>
#include "app/app_types.h"

typedef enum
{
    NORMAL_WAIT_START = 0,
    NORMAL_START_FLASH,
    NORMAL_TRACK_OUT,
    NORMAL_RED_WAIT,
    NORMAL_OBSTACLE_HOLD,
    NORMAL_AVOID_TURN,
    NORMAL_RETURN_CLEAR,
    NORMAL_RETURN,
    NORMAL_COMPLETE,
    NORMAL_FAULT
} NormalMissionState;

typedef struct
{
    uint8_t start_flash_done;
    uint8_t red_detected;
    uint8_t green_detected;
    uint8_t obstacle_detected;
    uint8_t obstacle_cleared;
    uint8_t obstacle_sample_ready;
    uint8_t hazard_hold_done;
    uint8_t turn_complete;
    uint8_t fatal_error;
} NormalMissionInput;

void NormalMission_Init(void);
void NormalMission_Start(uint32_t now_ms);
void NormalMission_Stop(void);
MissionOutput NormalMission_Update(const NormalMissionInput *input, uint32_t now_ms);
NormalMissionState NormalMission_GetState(void);

#endif
