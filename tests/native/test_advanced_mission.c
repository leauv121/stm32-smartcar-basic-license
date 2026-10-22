#include "app/advanced_mission.h"
#include "test_assert.h"

static void feed_event(RouteEvent event, uint32_t now_ms)
{
    AdvancedMissionInput input = {0};
    input.route_event = event;
    (void)AdvancedMission_Update(&input, now_ms);
}

int main(void)
{
    AdvancedMissionInput input = {0};
    MissionOutput output;

    AdvancedMission_Init();
    AdvancedMission_Start(APP_MODE_CIRCLE, 0U);
    feed_event(ROUTE_POINT_B, 100U);
    feed_event(ROUTE_POINT_C, 200U);
    feed_event(ROUTE_POINT_D, 300U);
    feed_event(ROUTE_POINT_A, 400U);
    output = AdvancedMission_Update(&input, 401U);
    TEST_ASSERT_EQ_INT(ADVANCED_COMPLETE, AdvancedMission_GetState());
    TEST_ASSERT_EQ_INT(1, output.complete);

    AdvancedMission_Start(APP_MODE_STABLE, 1000U);
    feed_event(ROUTE_POINT_C, 1100U); feed_event(ROUTE_POINT_B, 1200U);
    feed_event(ROUTE_POINT_D, 1300U); feed_event(ROUTE_POINT_A, 1400U);
    feed_event(ROUTE_POINT_C, 1500U); feed_event(ROUTE_POINT_B, 1600U);
    feed_event(ROUTE_POINT_D, 1700U); feed_event(ROUTE_POINT_A, 1800U);
    feed_event(ROUTE_POINT_C, 1900U); feed_event(ROUTE_POINT_B, 2000U);
    feed_event(ROUTE_POINT_D, 2100U); feed_event(ROUTE_POINT_A, 2200U);
    TEST_ASSERT_EQ_INT(ADVANCED_COMPLETE, AdvancedMission_GetState());
    TEST_ASSERT_EQ_INT(3, AdvancedMission_GetLapCount());
    return test_failures;
}
