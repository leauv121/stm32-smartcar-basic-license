#include "config/app_config.h"
#include "test_assert.h"

int main(void)
{
    TEST_ASSERT_EQ_INT(1, APP_FEATURE_ERROR_MANAGER);
    TEST_ASSERT_EQ_INT(1, APP_FEATURE_DIAGNOSTIC_DISPLAY);
    return test_failures;
}
