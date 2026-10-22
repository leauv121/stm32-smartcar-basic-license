#ifndef FINAL_WORK_SAFETY_SERVICE_H
#define FINAL_WORK_SAFETY_SERVICE_H

#include <stdint.h>

#include "control/motion_types.h"

void SafetyService_Init(void);
MotionRequest SafetyService_Resolve(MotionRequest requested,
                                    uint8_t user_stop,
                                    uint8_t mission_complete);
void SafetyService_Acknowledge(void);
uint8_t SafetyService_IsStopLatched(void);

#endif
