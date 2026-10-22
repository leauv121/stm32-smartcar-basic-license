#include "services/error_manager.h"
#include "services/sensor_service.h"
#include "test_assert.h"

static void test_updates_produce_valid_timestamped_snapshot(void)
{
    const SensorSnapshot *snapshot;

    ErrorManager_Init();
    SensorService_Init();
    SensorService_UpdateLine(1234U, 2345U, 1U, 0U, 50U);
    SensorService_UpdateYaw(37.5f, 60U);

    snapshot = SensorService_GetSnapshot();
    TEST_ASSERT_EQ_INT(1, snapshot->line.valid);
    TEST_ASSERT_EQ_INT(1234, snapshot->line.left_adc);
    TEST_ASSERT_EQ_INT(2345, snapshot->line.right_adc);
    TEST_ASSERT_EQ_INT(1, snapshot->yaw.valid);
    TEST_ASSERT_EQ_INT(60, snapshot->yaw.updated_ms);
}

static void test_required_stale_yaw_reports_fatal_error(void)
{
    const ErrorRecord *record;

    ErrorManager_Init();
    SensorService_Init();
    SensorService_UpdateYaw(0.0f, 10U);
    SensorService_CheckTimeouts(200U, 1U);

    record = ErrorManager_GetHighest();
    TEST_ASSERT_TRUE(record != 0);
    TEST_ASSERT_EQ_INT(ERR_MODULE_MPU6050, record->module);
    TEST_ASSERT_EQ_INT(ERROR_LEVEL_FATAL, record->level);
}

static void test_disabled_optional_sources_do_not_report_errors(void)
{
    ErrorManager_Init();
    SensorService_Init();
    SensorService_SetSourceEnabled(SENSOR_SOURCE_COLOR, 0U);
    SensorService_SetSourceEnabled(SENSOR_SOURCE_ULTRASONIC, 0U);
    SensorService_CheckTimeouts(5000U, 0U);

    TEST_ASSERT_EQ_INT(0, ErrorManager_GetActiveCount());
}

int main(void)
{
    test_updates_produce_valid_timestamped_snapshot();
    test_required_stale_yaw_reports_fatal_error();
    test_disabled_optional_sources_do_not_report_errors();
    return test_failures;
}
