#ifndef FINAL_WORK_ADVANCED_MISSION_H
#define FINAL_WORK_ADVANCED_MISSION_H

#include <stdint.h>
#include "app/app_types.h"

typedef enum { ROUTE_EVENT_NONE = 0, ROUTE_POINT_A, ROUTE_POINT_B, ROUTE_POINT_C, ROUTE_POINT_D } RouteEvent;
typedef enum { ADVANCED_WAIT_START = 0, ADVANCED_RUNNING, ADVANCED_COMPLETE, ADVANCED_FAULT } AdvancedMissionState;
typedef struct { RouteEvent route_event; uint8_t fatal_error; } AdvancedMissionInput;

void AdvancedMission_Init(void);
void AdvancedMission_Start(AppMode mode, uint32_t now_ms);
void AdvancedMission_Stop(void);
MissionOutput AdvancedMission_Update(const AdvancedMissionInput *input, uint32_t now_ms);
AdvancedMissionState AdvancedMission_GetState(void);
uint8_t AdvancedMission_GetLapCount(void);

#endif
