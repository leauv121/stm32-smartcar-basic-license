#ifndef FINAL_WORK_TURN_CONTROL_H
#define FINAL_WORK_TURN_CONTROL_H

#include <stdint.h>

typedef enum
{
    TURN_LEFT = 1,
    TURN_RIGHT = -1
} TurnDirection;

typedef enum
{
    TURN_IDLE = 0,
    TURN_RUNNING,
    TURN_COMPLETE,
    TURN_SENSOR_ERROR,
    TURN_TIMEOUT
} TurnResult;

void TurnControl_Reset(void);
void TurnControl_Start(float current_yaw,
                       float target_deg,
                       TurnDirection direction,
                       uint32_t now_ms);
TurnResult TurnControl_Update(float current_yaw, uint8_t yaw_valid, uint32_t now_ms);
float TurnControl_GetProgress(void);

#endif
