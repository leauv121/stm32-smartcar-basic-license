#include <string.h>

#include "services/diagnostic_view.h"
#include "test_assert.h"

static void test_fatal_mpu_timeout_has_bounded_readable_lines(void)
{
    ErrorRecord record = {0};
    DiagnosticLines lines;
    uint8_t index;

    record.module = ERR_MODULE_MPU6050;
    record.code = ERROR_CODE_DATA_TIMEOUT;
    record.level = ERROR_LEVEL_FATAL;
    record.active = 1U;

    DiagnosticView_Format(&record, 0U, 3U, &lines);

    TEST_ASSERT_TRUE(strcmp(lines.line[0], "ERROR 01/03") == 0);
    TEST_ASSERT_TRUE(strcmp(lines.line[1], "MOD:MPU6050") == 0);
    TEST_ASSERT_TRUE(strcmp(lines.line[2], "CODE:DATA TIME") == 0);
    TEST_ASSERT_TRUE(strcmp(lines.line[3], "MOTOR STOP") == 0);
    for (index = 0U; index < 4U; index++)
    {
        TEST_ASSERT_TRUE(strlen(lines.line[index]) <= 16U);
    }
}

static void test_warning_uses_warning_title_and_sensor_action(void)
{
    ErrorRecord record = {0};
    DiagnosticLines lines;

    record.module = ERR_MODULE_COLOR;
    record.code = ERROR_CODE_COMMUNICATION;
    record.level = ERROR_LEVEL_WARNING;
    record.active = 1U;

    DiagnosticView_Format(&record, 0U, 1U, &lines);

    TEST_ASSERT_TRUE(strcmp(lines.line[0], "WARNING 01/01") == 0);
    TEST_ASSERT_TRUE(strcmp(lines.line[1], "MOD:COLOR") == 0);
    TEST_ASSERT_TRUE(strcmp(lines.line[3], "CHECK SENSOR") == 0);
}

int main(void)
{
    test_fatal_mpu_timeout_has_bounded_readable_lines();
    test_warning_uses_warning_title_and_sensor_action();
    return test_failures;
}
