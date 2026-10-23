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

int main(void)
{
    AppInput input = MakeInput();
    AppOutput output;

    ErrorManager_Init();
    AppController_Init();
    TEST_ASSERT_EQ_INT(APP_STATE_SELF_TEST, AppController_GetState());

    output = AppController_Update(&input, 10U);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);
    TEST_ASSERT_EQ_INT(APP_MODE_NONE, output.mode);

    input.command.type = APP_COMMAND_START;
    output = AppController_Update(&input, 20U);
    TEST_ASSERT_EQ_INT(APP_RESULT_MODE_REQUIRED, output.command_result);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);

    input.command.type = APP_COMMAND_SET_MODE;
    input.command.mode = APP_MODE_NORMAL;
    output = AppController_Update(&input, 30U);
    TEST_ASSERT_EQ_INT(APP_RESULT_OK, output.command_result);
    TEST_ASSERT_EQ_INT(APP_MODE_NORMAL, output.mode);

    input.command.type = APP_COMMAND_START;
    output = AppController_Update(&input, 40U);
    TEST_ASSERT_EQ_INT(APP_STATE_RUNNING, output.state);

    input.command.type = APP_COMMAND_SET_MODE;
    input.command.mode = APP_MODE_EIGHT;
    output = AppController_Update(&input, 50U);
    TEST_ASSERT_EQ_INT(APP_RESULT_BUSY, output.command_result);
    TEST_ASSERT_EQ_INT(APP_MODE_NORMAL, output.mode);

    input.command.type = APP_COMMAND_STOP;
    input.user_button_pressed = 1U;
    output = AppController_Update(&input, 60U);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);
    TEST_ASSERT_TRUE(output.user_stop_requested);
    input.user_button_pressed = 0U;

    input.stop_latched = 1U;
    input.command.type = APP_COMMAND_START;
    output = AppController_Update(&input, 65U);
    TEST_ASSERT_EQ_INT(APP_RESULT_ACK_REQUIRED, output.command_result);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);
    input.command.type = APP_COMMAND_NONE;
    input.user_button_pressed = 1U;
    output = AppController_Update(&input, 65U);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);
    input.user_button_pressed = 0U;
    input.command.type = APP_COMMAND_ACK;
    output = AppController_Update(&input, 66U);
    TEST_ASSERT_EQ_INT(APP_RESULT_OK, output.command_result);
    input.stop_latched = 0U;

    input.command.type = APP_COMMAND_SET_MODE;
    input.command.mode = APP_MODE_CIRCLE;
    (void)AppController_Update(&input, 70U);
    input.command.type = APP_COMMAND_NONE;
    input.user_button_pressed = 1U;
    output = AppController_Update(&input, 80U);
    TEST_ASSERT_EQ_INT(APP_STATE_RUNNING, output.state);
    input.user_button_pressed = 0U;

    ErrorManager_Report(ERR_MODULE_MPU6050, ERROR_CODE_DATA_TIMEOUT,
                        ERROR_LEVEL_FATAL, 90U, 0U);
    output = AppController_Update(&input, 90U);
    TEST_ASSERT_EQ_INT(APP_STATE_FAULT, output.state);
    TEST_ASSERT_EQ_INT(MOTION_STOP, output.motion.mode);

    input.command.type = APP_COMMAND_START;
    output = AppController_Update(&input, 100U);
    TEST_ASSERT_EQ_INT(APP_RESULT_FATAL_LOCKED, output.command_result);
    TEST_ASSERT_EQ_INT(APP_STATE_FAULT, output.state);

    ErrorManager_Clear(ERR_MODULE_MPU6050, ERROR_CODE_DATA_TIMEOUT, 110U);
    input.command.type = APP_COMMAND_ACK;
    output = AppController_Update(&input, 110U);
    TEST_ASSERT_EQ_INT(APP_RESULT_OK, output.command_result);
    TEST_ASSERT_EQ_INT(APP_STATE_WAIT, output.state);

    input.command.type = APP_COMMAND_STATUS;
    output = AppController_Update(&input, 120U);
    TEST_ASSERT_TRUE(output.status_requested);
    input.command.type = APP_COMMAND_ERRORS;
    output = AppController_Update(&input, 130U);
    TEST_ASSERT_TRUE(output.errors_requested);

    return test_failures;
}
