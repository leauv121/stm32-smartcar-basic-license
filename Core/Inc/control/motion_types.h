#ifndef FINAL_WORK_MOTION_TYPES_H
#define FINAL_WORK_MOTION_TYPES_H

#include <stdint.h>

typedef enum
{
    MOTION_STOP = 0,
    MOTION_DRIVE,
    MOTION_TURN_IN_PLACE
} MotionMode;

typedef struct
{
    MotionMode mode;
    int16_t left_target;
    int16_t right_target;
} MotionRequest;

static inline MotionRequest MotionRequest_Stop(void)
{
    MotionRequest request;
    request.mode = MOTION_STOP;
    request.left_target = 0;
    request.right_target = 0;
    return request;
}

#endif
