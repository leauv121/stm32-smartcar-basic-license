#ifndef FINAL_WORK_APP_TYPES_H
#define FINAL_WORK_APP_TYPES_H

#include <stdint.h>

#include "control/motion_types.h"
#include "services/indicator_service.h"

typedef enum
{
    APP_MODE_NONE = 0,
    APP_MODE_NORMAL,
    APP_MODE_CIRCLE,
    APP_MODE_EIGHT,
    APP_MODE_STABLE,
    APP_MODE_CHALLENGE
} AppMode;

typedef struct
{
    MotionRequest motion;
    IndicatorPattern indicator;
    uint8_t complete;
    uint8_t point_reached;
} MissionOutput;

#endif
