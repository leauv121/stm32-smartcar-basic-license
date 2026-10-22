#ifndef FINAL_WORK_SCHEDULER_H
#define FINAL_WORK_SCHEDULER_H

#include <stdint.h>

typedef struct
{
    uint8_t tick_10ms;
    uint8_t tick_20ms;
    uint8_t tick_50ms;
    uint8_t tick_100ms;
    uint8_t tick_200ms;
    uint8_t tick_500ms;
} SchedulerEvents;

void Scheduler_Init(uint32_t now_ms);
SchedulerEvents Scheduler_Update(uint32_t now_ms);

#endif
