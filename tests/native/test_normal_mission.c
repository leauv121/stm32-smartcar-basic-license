#include "app/normal_mission.h"
#include "test_assert.h"

static NormalMissionInput no_event(void)
{
    NormalMissionInput input = {0};
    return input;
}

int main(void)
{
    NormalMissionInput input;
    MissionOutput output;

    NormalMission_Init();
    NormalMission_Start(0U);
    TEST_ASSERT_EQ_INT(NORMAL_START_FLASH, NormalMission_GetState());

    input = no_event();
    input.start_flash_done = 1U;
    output = NormalMission_Update(&input, 3000U);
    TEST_ASSERT_EQ_INT(NORMAL_TRACK_OUT, NormalMission_GetState());
    TEST_ASSERT_EQ_INT(MOTION_DRIVE, output.motion.mode);

    input = no_event();
    input.red_detected = 1U;
    output = NormalMission_Update(&input, 3100U);
    TEST_ASSERT_EQ_INT(NORMAL_RED_WAIT, NormalMission_GetState());
    TEST_ASSERT_EQ_INT(MOTION_STOP, output.motion.mode);

    input = no_event();
    input.green_detected = 1U;
    NormalMission_Update(&input, 3200U);
    TEST_ASSERT_EQ_INT(NORMAL_TRACK_OUT, NormalMission_GetState());

    input = no_event();
    input.obstacle_detected = 1U;
    NormalMission_Update(&input, 3300U);
    TEST_ASSERT_EQ_INT(NORMAL_OBSTACLE_HOLD, NormalMission_GetState());

    input = no_event();
    input.hazard_hold_done = 1U;
    NormalMission_Update(&input, 4300U);
    TEST_ASSERT_EQ_INT(NORMAL_AVOID_TURN, NormalMission_GetState());

    input = no_event();
    input.turn_complete = 1U;
    NormalMission_Update(&input, 4500U);
    TEST_ASSERT_EQ_INT(NORMAL_RETURN, NormalMission_GetState());

    input = no_event();
    input.home_detected = 1U;
    output = NormalMission_Update(&input, 5000U);
    TEST_ASSERT_EQ_INT(NORMAL_COMPLETE, NormalMission_GetState());
    TEST_ASSERT_EQ_INT(1, output.complete);
    TEST_ASSERT_EQ_INT(MOTION_STOP, output.motion.mode);
    return test_failures;
}
