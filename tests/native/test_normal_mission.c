#include "app/normal_mission.h"
#include "test_assert.h"

static NormalMissionInput no_event(void)
{
    NormalMissionInput input = {0};
    return input;
}

static void enter_return_clear(void)
{
    NormalMissionInput input;

    NormalMission_Init();
    NormalMission_Start(0U);
    input = no_event();
    input.start_flash_done = 1U;
    (void)NormalMission_Update(&input, 100U);
    input = no_event();
    input.obstacle_detected = 1U;
    (void)NormalMission_Update(&input, 200U);
    input = no_event();
    input.hazard_hold_done = 1U;
    (void)NormalMission_Update(&input, 1200U);
    input = no_event();
    input.turn_complete = 1U;
    (void)NormalMission_Update(&input, 1500U);
    input = no_event();
    input.line_reacquired = 1U;
    (void)NormalMission_Update(&input, 1600U);
}

static void test_stop_and_restart_clear_partial_return_confirmation(void)
{
    NormalMissionInput input = no_event();

    enter_return_clear();
    input.obstacle_sample_ready = 1U;
    input.obstacle_cleared = 1U;
    (void)NormalMission_Update(&input, 1600U);
    (void)NormalMission_Update(&input, 1700U);

    NormalMission_Stop();
    enter_return_clear();
    (void)NormalMission_Update(&input, 1800U);
    TEST_ASSERT_EQ_INT(NORMAL_RETURN_CLEAR, NormalMission_GetState());
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

    /* 红灯等待不能被时间或障碍物事件解除，只能由绿灯解除。 */
    input = no_event();
    input.obstacle_detected = 1U;
    output = NormalMission_Update(&input, 10000U);
    TEST_ASSERT_EQ_INT(NORMAL_RED_WAIT, NormalMission_GetState());
    TEST_ASSERT_EQ_INT(MOTION_STOP, output.motion.mode);

    input = no_event();
    input.green_detected = 1U;
    NormalMission_Update(&input, 10100U);
    TEST_ASSERT_EQ_INT(NORMAL_TRACK_OUT, NormalMission_GetState());

    input = no_event();
    input.obstacle_detected = 1U;
    NormalMission_Update(&input, 3300U);
    TEST_ASSERT_EQ_INT(NORMAL_OBSTACLE_HOLD, NormalMission_GetState());

    input = no_event();
    input.hazard_hold_done = 1U;
    output = NormalMission_Update(&input, 4300U);
    TEST_ASSERT_EQ_INT(NORMAL_AVOID_TURN, NormalMission_GetState());
    TEST_ASSERT_EQ_INT(3200, output.motion.left_target);
    TEST_ASSERT_EQ_INT(-3200, output.motion.right_target);

    input = no_event();
    input.turn_complete = 1U;
    output = NormalMission_Update(&input, 4500U);
    TEST_ASSERT_EQ_INT(NORMAL_REACQUIRE_LINE, NormalMission_GetState());
    TEST_ASSERT_EQ_INT(MOTION_DRIVE, output.motion.mode);
    TEST_ASSERT_EQ_INT(2400, output.motion.left_target);
    TEST_ASSERT_EQ_INT(700, output.motion.right_target);

    input = no_event();
    input.line_reacquired = 1U;
    NormalMission_Update(&input, 4550U);
    TEST_ASSERT_EQ_INT(NORMAL_RETURN_CLEAR, NormalMission_GetState());

    /* 掉头后必须连续三次确认已离开原障碍物，才能开始等待返程终点。 */
    input = no_event();
    input.obstacle_sample_ready = 1U;
    input.obstacle_cleared = 1U;
    NormalMission_Update(&input, 4600U);
    input.obstacle_sample_ready = 0U;
    NormalMission_Update(&input, 4700U);
    TEST_ASSERT_EQ_INT(NORMAL_RETURN_CLEAR, NormalMission_GetState());
    input.obstacle_sample_ready = 1U;
    input.obstacle_cleared = 0U;
    NormalMission_Update(&input, 4800U);
    TEST_ASSERT_EQ_INT(NORMAL_RETURN_CLEAR, NormalMission_GetState());
    input.obstacle_cleared = 1U;
    NormalMission_Update(&input, 4900U);
    NormalMission_Update(&input, 5000U);
    TEST_ASSERT_EQ_INT(NORMAL_RETURN_CLEAR, NormalMission_GetState());
    NormalMission_Update(&input, 5100U);
    TEST_ASSERT_EQ_INT(NORMAL_RETURN, NormalMission_GetState());

    /* 离开后再次连续三次检测到障碍物，才认为返回出发点。 */
    input = no_event();
    input.obstacle_sample_ready = 1U;
    input.obstacle_detected = 1U;
    NormalMission_Update(&input, 5200U);
    input.obstacle_sample_ready = 0U;
    NormalMission_Update(&input, 5300U);
    TEST_ASSERT_EQ_INT(NORMAL_RETURN, NormalMission_GetState());
    input.obstacle_sample_ready = 1U;
    input.obstacle_detected = 0U;
    NormalMission_Update(&input, 5400U);
    input.obstacle_detected = 1U;
    NormalMission_Update(&input, 5500U);
    NormalMission_Update(&input, 5600U);
    TEST_ASSERT_EQ_INT(NORMAL_RETURN, NormalMission_GetState());
    output = NormalMission_Update(&input, 5700U);
    TEST_ASSERT_EQ_INT(NORMAL_COMPLETE, NormalMission_GetState());
    TEST_ASSERT_EQ_INT(1, output.complete);
    TEST_ASSERT_EQ_INT(MOTION_STOP, output.motion.mode);
    test_stop_and_restart_clear_partial_return_confirmation();
    return test_failures;
}
