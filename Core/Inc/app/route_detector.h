#ifndef FINAL_WORK_ROUTE_DETECTOR_H
#define FINAL_WORK_ROUTE_DETECTOR_H

#include <stdint.h>

#include "app/advanced_mission.h"

typedef struct
{
    RouteEvent candidate;
    uint8_t candidate_valid;
} RouteDetectorInput;

/* 路点识别只负责去抖和单次上报，具体传感器阈值由硬件适配层提供。 */
void RouteDetector_Init(void);
void RouteDetector_Reset(void);
RouteEvent RouteDetector_Update(const RouteDetectorInput *input);

#endif
