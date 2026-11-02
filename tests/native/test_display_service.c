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
    model.outer_right = 1U;
    model.outer_left = 0U;
    model.line_target_left = 1400;
    model.line_target_right = 4500;
    model.line_mode = 'L';
    model.distance_cm = 25.4f;
    model.battery_v = 7.40f;
    model.yaw = -12.5f;
    model.color = SENSOR_COLOR_GREEN;
    DisplayService_FormatNormal(&model, &lines);

    TEST_ASSERT_TRUE(strstr(lines.line[0], "L:1.2 R:1.3") != 0);
    TEST_ASSERT_TRUE(strstr(lines.line[1], "D:25.4cm C:GRN") != 0);
    TEST_ASSERT_TRUE(strstr(lines.line[2], "B:7.40V") != 0);
    TEST_ASSERT_TRUE(strstr(lines.line[3], "RUN EIGHT") != 0);
    TEST_ASSERT_EQ_INT(16, (int)strlen(lines.line[0]));
    TEST_ASSERT_EQ_INT(16, (int)strlen(lines.line[3]));

    model.distance_cm = 0.0f;
    model.battery_v = 0.0f;
    model.color = SENSOR_COLOR_UNKNOWN;
    DisplayService_FormatNormal(&model, &lines);
    TEST_ASSERT_TRUE(strstr(lines.line[1], "D:N/A C:UNK") != 0);
    TEST_ASSERT_TRUE(strstr(lines.line[2], "B:N/A") != 0);

    return test_failures;
}
