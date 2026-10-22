#include "services/scheduler.h"

typedef struct
{
    uint32_t last_10ms;
    uint32_t last_20ms;
    uint32_t last_50ms;
    uint32_t last_100ms;
    uint32_t last_200ms;
    uint32_t last_500ms;
} SchedulerState;

static SchedulerState scheduler_state;

static uint8_t Scheduler_Due(uint32_t now_ms,
                             uint32_t *last_ms,
                             uint32_t interval_ms)
{
    if ((uint32_t)(now_ms - *last_ms) >= interval_ms)
    {
        /* 不补跑积压周期，避免阻塞结束后连续执行多次控制。 */
        *last_ms = now_ms;
        return 1U;
    }
    return 0U;
}

void Scheduler_Init(uint32_t now_ms)
{
    scheduler_state.last_10ms = now_ms;
    scheduler_state.last_20ms = now_ms;
    scheduler_state.last_50ms = now_ms;
    scheduler_state.last_100ms = now_ms;
    scheduler_state.last_200ms = now_ms;
    scheduler_state.last_500ms = now_ms;
}

SchedulerEvents Scheduler_Update(uint32_t now_ms)
{
    SchedulerEvents events;

    events.tick_10ms = Scheduler_Due(now_ms, &scheduler_state.last_10ms, 10U);
    events.tick_20ms = Scheduler_Due(now_ms, &scheduler_state.last_20ms, 20U);
    events.tick_50ms = Scheduler_Due(now_ms, &scheduler_state.last_50ms, 50U);
    events.tick_100ms = Scheduler_Due(now_ms, &scheduler_state.last_100ms, 100U);
    events.tick_200ms = Scheduler_Due(now_ms, &scheduler_state.last_200ms, 200U);
    events.tick_500ms = Scheduler_Due(now_ms, &scheduler_state.last_500ms, 500U);
    return events;
}
