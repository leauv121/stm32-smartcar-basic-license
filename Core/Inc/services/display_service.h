#ifndef FINAL_WORK_DISPLAY_SERVICE_H
#define FINAL_WORK_DISPLAY_SERVICE_H

#include "app/app_controller.h"
#include "services/sensor_service.h"

typedef struct
{
    AppState state;
    AppMode mode;
    float left_speed;
    float right_speed;
    uint8_t outer_right;
    uint8_t outer_left;
    int16_t line_target_left;
    int16_t line_target_right;
    char line_mode;
    float distance_cm;
    float battery_v;
    float yaw;
    SensorColor color;
} DisplayModel;

typedef struct
{
    char line[4][17];
} DisplayLines;

void DisplayService_FormatNormal(const DisplayModel *model, DisplayLines *out);

#endif
