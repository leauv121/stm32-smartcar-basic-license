#include <stdint.h>

#include "services/scheduler.h"
#include "test_assert.h"

static void test_events_fire_once_at_boundaries(void)
{
    SchedulerEvents events;

    Scheduler_Init(0U);
    events = Scheduler_Update(19U);
    TEST_ASSERT_EQ_INT(0, events.tick_20ms);
    events = Scheduler_Update(20U);
    TEST_ASSERT_EQ_INT(1, events.tick_20ms);
    events = Scheduler_Update(21U);
    TEST_ASSERT_EQ_INT(0, events.tick_20ms);
    events = Scheduler_Update(500U);
    TEST_ASSERT_EQ_INT(1, events.tick_20ms);
    TEST_ASSERT_EQ_INT(1, events.tick_50ms);
    TEST_ASSERT_EQ_INT(1, events.tick_100ms);
    TEST_ASSERT_EQ_INT(1, events.tick_200ms);
    TEST_ASSERT_EQ_INT(1, events.tick_500ms);
}

static void test_tick_wraparound_is_safe(void)
{
    SchedulerEvents events;

    Scheduler_Init(UINT32_MAX - 15U);
    events = Scheduler_Update(4U);
    TEST_ASSERT_EQ_INT(1, events.tick_20ms);
    TEST_ASSERT_EQ_INT(0, events.tick_50ms);
}

int main(void)
{
    test_events_fire_once_at_boundaries();
    test_tick_wraparound_is_safe();
    return test_failures;
}
