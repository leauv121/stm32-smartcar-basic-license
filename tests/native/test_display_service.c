#include <string.h>

#include "services/display_service.h"
#include "test_assert.h"

int main(void)
{
    DisplayModel model = {0};
    DisplayLines lines;

    model.state = APP_STATE_RUNNING;
    model.mode = APP_MODE_EIGHT;
    model.left_speed = 1.2f;
    model.right_speed = 1.3f;
    model.distance_cm = 25.4f;
    model.yaw = -12.5f;
    model.color = SENSOR_COLOR_GREEN;
    DisplayService_FormatNormal(&model, &lines);

    TEST_ASSERT_TRUE(strstr(lines.line[0], "RUN EIGHT") != 0);
    TEST_ASSERT_TRUE(strstr(lines.line[1], "L:1.2 R:1.3") != 0);
    TEST_ASSERT_TRUE(strstr(lines.line[2], "D:25.4 GREEN") != 0);
    TEST_ASSERT_TRUE(strstr(lines.line[3], "Y:-12.5") != 0);
    TEST_ASSERT_EQ_INT(16, (int)strlen(lines.line[0]));
    TEST_ASSERT_EQ_INT(16, (int)strlen(lines.line[3]));

    return test_failures;
}
