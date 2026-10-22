#include <stdint.h>

#include "services/error_manager.h"
#include "test_assert.h"

static void test_duplicate_report_updates_existing_record(void)
{
    const ErrorRecord *record;

    ErrorManager_Init();
    ErrorManager_Report(ERR_MODULE_ADC,
                        ERROR_CODE_COMMUNICATION,
                        ERROR_LEVEL_WARNING,
                        100U,
                        1U);
    ErrorManager_Report(ERR_MODULE_ADC,
                        ERROR_CODE_COMMUNICATION,
                        ERROR_LEVEL_WARNING,
                        150U,
                        1U);

    record = ErrorManager_GetHighest();
    TEST_ASSERT_EQ_INT(1, ErrorManager_GetActiveCount());
    TEST_ASSERT_TRUE(record != 0);
    TEST_ASSERT_EQ_INT(100, record->first_seen_ms);
    TEST_ASSERT_EQ_INT(150, record->last_seen_ms);
    TEST_ASSERT_EQ_INT(2, record->occurrence_count);
}

static void test_fatal_error_has_priority_and_can_be_cleared(void)
{
    const ErrorRecord *record;

    ErrorManager_Init();
    ErrorManager_Report(ERR_MODULE_COLOR,
                        ERROR_CODE_DATA_TIMEOUT,
                        ERROR_LEVEL_WARNING,
                        10U,
                        1U);
    ErrorManager_Report(ERR_MODULE_MPU6050,
                        ERROR_CODE_DATA_TIMEOUT,
                        ERROR_LEVEL_FATAL,
                        20U,
                        0U);

    record = ErrorManager_GetHighest();
    TEST_ASSERT_TRUE(record != 0);
    TEST_ASSERT_EQ_INT(ERROR_LEVEL_FATAL, record->level);
    TEST_ASSERT_EQ_INT(ERR_MODULE_MPU6050, record->module);
    TEST_ASSERT_EQ_INT(1, ErrorManager_HasFatal());

    ErrorManager_Clear(ERR_MODULE_MPU6050, ERROR_CODE_DATA_TIMEOUT, 30U);
    TEST_ASSERT_EQ_INT(0, ErrorManager_HasFatal());
    TEST_ASSERT_EQ_INT(1, ErrorManager_GetActiveCount());
}

static void test_equal_priority_errors_rotate_every_two_seconds(void)
{
    const ErrorRecord *first;
    const ErrorRecord *second;

    ErrorManager_Init();
    ErrorManager_Report(ERR_MODULE_ADC,
                        ERROR_CODE_VALUE_INVALID,
                        ERROR_LEVEL_WARNING,
                        0U,
                        1U);
    ErrorManager_Report(ERR_MODULE_COLOR,
                        ERROR_CODE_COMMUNICATION,
                        ERROR_LEVEL_WARNING,
                        10U,
                        1U);

    first = ErrorManager_GetDisplayRecord(0U);
    second = ErrorManager_GetDisplayRecord(2000U);

    TEST_ASSERT_TRUE(first != 0);
    TEST_ASSERT_TRUE(second != 0);
    TEST_ASSERT_EQ_INT(ERR_MODULE_ADC, first->module);
    TEST_ASSERT_EQ_INT(ERR_MODULE_COLOR, second->module);
}

int main(void)
{
    test_duplicate_report_updates_existing_record();
    test_fatal_error_has_priority_and_can_be_cleared();
    test_equal_priority_errors_rotate_every_two_seconds();
    return test_failures;
}
