#include "control/turn_control.h"
#include "test_assert.h"

static void test_yaw_wraparound_accumulates_small_delta(void)
{
    TurnResult result;

    TurnControl_Start(179.0f, 10.0f, TURN_LEFT, 0U);
    result = TurnControl_Update(-179.0f, 1U, 10U);
    TEST_ASSERT_EQ_INT(TURN_RUNNING, result);
    TEST_ASSERT_TRUE(TurnControl_GetProgress() > 1.9f);
    TEST_ASSERT_TRUE(TurnControl_GetProgress() < 2.1f);
}

static void test_right_turn_completes_after_target_angle(void)
{
    TurnResult result;

    TurnControl_Start(0.0f, 180.0f, TURN_RIGHT, 0U);
    result = TurnControl_Update(-90.0f, 1U, 100U);
    TEST_ASSERT_EQ_INT(TURN_RUNNING, result);
    result = TurnControl_Update(179.0f, 1U, 200U);
    TEST_ASSERT_EQ_INT(TURN_COMPLETE, result);
}

static void test_reverse_motion_is_subtracted_from_turn_progress(void)
{
    TurnResult result;

    TurnControl_Start(0.0f, 180.0f, TURN_RIGHT, 0U);
    result = TurnControl_Update(-100.0f, 1U, 100U);
    TEST_ASSERT_EQ_INT(TURN_RUNNING, result);

    /* 回摆 50 度后，净右转角应从 100 度退回到 50 度。 */
    result = TurnControl_Update(-50.0f, 1U, 200U);
    TEST_ASSERT_EQ_INT(TURN_RUNNING, result);
    TEST_ASSERT_TRUE(TurnControl_GetProgress() > 49.9f);
    TEST_ASSERT_TRUE(TurnControl_GetProgress() < 50.1f);

    result = TurnControl_Update(-130.0f, 1U, 300U);
    TEST_ASSERT_EQ_INT(TURN_RUNNING, result);
    TEST_ASSERT_TRUE(TurnControl_GetProgress() > 129.9f);
    TEST_ASSERT_TRUE(TurnControl_GetProgress() < 130.1f);

    result = TurnControl_Update(-180.0f, 1U, 400U);
    TEST_ASSERT_EQ_INT(TURN_COMPLETE, result);
}

static void test_invalid_yaw_and_timeout_stop_turn(void)
{
    TurnResult result;

    TurnControl_Start(0.0f, 90.0f, TURN_LEFT, 0U);
    result = TurnControl_Update(0.0f, 0U, 10U);
    TEST_ASSERT_EQ_INT(TURN_SENSOR_ERROR, result);

    TurnControl_Start(0.0f, 90.0f, TURN_LEFT, 0U);
    result = TurnControl_Update(10.0f, 1U, 6000U);
    TEST_ASSERT_EQ_INT(TURN_TIMEOUT, result);
}

int main(void)
{
    test_yaw_wraparound_accumulates_small_delta();
    test_right_turn_completes_after_target_angle();
    test_reverse_motion_is_subtracted_from_turn_progress();
    test_invalid_yaw_and_timeout_stop_turn();
    return test_failures;
}
