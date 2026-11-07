#include <string.h>

#include "services/line_telemetry.h"
#include "test_assert.h"

int main(void)
{
    LineTelemetrySample sample = {
        12540U,
        3650U,
        820U,
        0U,
        1U,
        -2830,
        870,
        6500,
        'P'
    };
    char text[96];

    TEST_ASSERT_TRUE(LineTelemetry_Format(&sample, text, sizeof(text)) > 0);
    TEST_ASSERT_TRUE(strcmp(text,
        "LF,12540,3650,820,0,1,-2830,870,6500,P\r\n") == 0);
    TEST_ASSERT_EQ_INT(0, LineTelemetry_Format(0, text, sizeof(text)));
    TEST_ASSERT_EQ_INT(0, LineTelemetry_Format(&sample, 0, sizeof(text)));
    TEST_ASSERT_EQ_INT(0, LineTelemetry_Format(&sample, text, 8U));

    return test_failures;
}
