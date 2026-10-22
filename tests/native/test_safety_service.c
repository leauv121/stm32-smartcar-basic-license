#include "control/motion_types.h"
#include "services/error_manager.h"
#include "services/safety_service.h"
#include "test_assert.h"

static MotionRequest drive_request(void)
{
    MotionRequest request;
    request.mode = MOTION_DRIVE;
    request.left_target = 3200;
    request.right_target = 3100;
    return request;
}

static void test_normal_request_passes_through(void)
{
    MotionRequest output;

    ErrorManager_Init();
    SafetyService_Init();
    output = SafetyService_Resolve(drive_request(), 0U, 0U);

    TEST_ASSERT_EQ_INT(MOTION_DRIVE, output.mode);
    TEST_ASSERT_EQ_INT(3200, output.left_target);
    TEST_ASSERT_EQ_INT(3100, output.right_target);
}

static void test_fatal_error_latches_stop_until_acknowledged(void)
{
    MotionRequest output;

    ErrorManager_Init();
    SafetyService_Init();
    ErrorManager_Report(ERR_MODULE_MPU6050,
                        ERROR_CODE_DATA_TIMEOUT,
                        ERROR_LEVEL_FATAL,
                        10U,
                        0U);

    output = SafetyService_Resolve(drive_request(), 0U, 0U);
    TEST_ASSERT_EQ_INT(MOTION_STOP, output.mode);

    ErrorManager_Clear(ERR_MODULE_MPU6050, ERROR_CODE_DATA_TIMEOUT, 20U);
    output = SafetyService_Resolve(drive_request(), 0U, 0U);
    TEST_ASSERT_EQ_INT(MOTION_STOP, output.mode);

    SafetyService_Acknowledge();
    output = SafetyService_Resolve(drive_request(), 0U, 0U);
    TEST_ASSERT_EQ_INT(MOTION_DRIVE, output.mode);
}

static void test_user_stop_and_completion_request_stop(void)
{
    MotionRequest output;

    ErrorManager_Init();
    SafetyService_Init();
    output = SafetyService_Resolve(drive_request(), 1U, 0U);
    TEST_ASSERT_EQ_INT(MOTION_STOP, output.mode);

    SafetyService_Init();
    output = SafetyService_Resolve(drive_request(), 0U, 1U);
    TEST_ASSERT_EQ_INT(MOTION_STOP, output.mode);
}

int main(void)
{
    test_normal_request_passes_through();
    test_fatal_error_latches_stop_until_acknowledged();
    test_user_stop_and_completion_request_stop();
    return test_failures;
}
