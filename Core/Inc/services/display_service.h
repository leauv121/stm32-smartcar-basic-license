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
