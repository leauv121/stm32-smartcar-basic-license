#include "app/app_controller.h"
#include "services/error_manager.h"
#include "test_assert.h"

static AppInput MakeInput(void)
{
    AppInput input = {0};
    input.self_test_complete = 1U;
    input.self_test_safe = 1U;
    return input;
}

static void test_serial_start_uses_normal_mode_by_default(void)
{
    AppInput input = MakeInput();
    AppOutput output;

    ErrorManager_Init();
    AppController_Init();
    TEST_ASSERT_EQ_INT(APP_STATE_SELF_TEST, AppController_GetState());

    output = AppController_Update(&input, 10U);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);
    TEST_ASSERT_EQ_INT(APP_MODE_NORMAL, output.mode);

    input.command.type = APP_COMMAND_START;
    output = AppController_Update(&input, 20U);
    TEST_ASSERT_EQ_INT(APP_RESULT_OK, output.command_result);
    TEST_ASSERT_EQ_INT(APP_STATE_RUNNING, output.state);
    TEST_ASSERT_EQ_INT(APP_MODE_NORMAL, output.mode);
}

static void test_user_button_starts_and_stops_normal_mode(void)
{
    AppInput input = MakeInput();
    AppOutput output;

    ErrorManager_Init();
    AppController_Init();
    (void)AppController_Update(&input, 10U);

    input.user_button_pressed = 1U;
    output = AppController_Update(&input, 20U);
    TEST_ASSERT_EQ_INT(APP_STATE_RUNNING, output.state);
    TEST_ASSERT_EQ_INT(APP_MODE_NORMAL, output.mode);

    output = AppController_Update(&input, 30U);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);
    TEST_ASSERT_TRUE(output.user_stop_requested);
}

static void test_advanced_mode_is_rejected(void)
{
    AppInput input = MakeInput();
    AppOutput output;

    ErrorManager_Init();
    AppController_Init();
    (void)AppController_Update(&input, 10U);

    input.command.type = APP_COMMAND_SET_MODE;
    input.command.mode = APP_MODE_CIRCLE;
    output = AppController_Update(&input, 30U);
    TEST_ASSERT_EQ_INT(APP_RESULT_INVALID_MODE, output.command_result);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);
    TEST_ASSERT_EQ_INT(APP_MODE_NORMAL, output.mode);
}

static void test_stop_latch_and_fatal_error_still_block_start(void)
{
    AppInput input = MakeInput();
    AppOutput output;

    ErrorManager_Init();
    AppController_Init();
    (void)AppController_Update(&input, 10U);

    input.stop_latched = 1U;
    input.command.type = APP_COMMAND_START;
    output = AppController_Update(&input, 20U);
    TEST_ASSERT_EQ_INT(APP_RESULT_ACK_REQUIRED, output.command_result);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);

    input.command.type = APP_COMMAND_ACK;
    output = AppController_Update(&input, 30U);
    TEST_ASSERT_EQ_INT(APP_RESULT_OK, output.command_result);
    input.stop_latched = 0U;

    ErrorManager_Report(ERR_MODULE_MPU6050, ERROR_CODE_DATA_TIMEOUT,
                        ERROR_LEVEL_FATAL, 40U, 0U);
    input.command.type = APP_COMMAND_START;
    output = AppController_Update(&input, 40U);
    TEST_ASSERT_EQ_INT(APP_RESULT_FATAL_LOCKED, output.command_result);
    TEST_ASSERT_EQ_INT(APP_STATE_FAULT, output.state);

    ErrorManager_Clear(ERR_MODULE_MPU6050, ERROR_CODE_DATA_TIMEOUT, 50U);
    input.command.type = APP_COMMAND_ACK;
    output = AppController_Update(&input, 50U);
    TEST_ASSERT_EQ_INT(APP_RESULT_OK, output.command_result);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);
}

int main(void)
{
    test_serial_start_uses_normal_mode_by_default();
    test_user_button_starts_and_stops_normal_mode();
    test_advanced_mode_is_rejected();
    test_stop_latch_and_fatal_error_still_block_start();

    return test_failures;
}
