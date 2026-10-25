#include "services/sensor_service.h"

#include <string.h>

#include "services/error_manager.h"

#define SENSOR_YAW_TIMEOUT_MS         100U
#define SENSOR_LINE_TIMEOUT_MS        100U
#define SENSOR_COLOR_TIMEOUT_MS       500U
#define SENSOR_ULTRASONIC_TIMEOUT_MS  500U

static SensorSnapshot sensor_snapshot;
static uint8_t source_enabled[SENSOR_SOURCE_COUNT];
static uint8_t timeout_reported[SENSOR_SOURCE_COUNT];

static void SensorService_ReportTimeoutOnce(SensorSource source,
                                            ErrorModule module,
                                            ErrorLevel level,
                                            uint32_t now_ms,
                                            uint8_t auto_recover)
{
    if (timeout_reported[source] == 0U)
    {
        ErrorManager_Report(module, ERROR_CODE_DATA_TIMEOUT, level,
                            now_ms, auto_recover);
        timeout_reported[source] = 1U;
    }
}

void SensorService_Init(void)
{
    uint8_t index;

    memset(&sensor_snapshot, 0, sizeof(sensor_snapshot));
    for (index = 0U; index < SENSOR_SOURCE_COUNT; index++)
    {
        source_enabled[index] = 1U;
        timeout_reported[index] = 0U;
    }
}

void SensorService_SetSourceEnabled(SensorSource source, uint8_t enabled)
{
    if (source < SENSOR_SOURCE_COUNT)
    {
        source_enabled[source] = (enabled != 0U) ? 1U : 0U;
    }
}

void SensorService_UpdateLine(uint16_t left_adc,
                              uint16_t right_adc,
                              uint8_t outer_right,
                              uint8_t outer_left,
                              uint32_t now_ms)
{
    sensor_snapshot.line.left_adc = left_adc;
    sensor_snapshot.line.right_adc = right_adc;
    sensor_snapshot.line.outer_right = outer_right;
    sensor_snapshot.line.outer_left = outer_left;
    sensor_snapshot.line.valid = 1U;
    sensor_snapshot.line.updated_ms = now_ms;
    timeout_reported[SENSOR_SOURCE_LINE] = 0U;
    ErrorManager_Clear(ERR_MODULE_LINE_SENSOR, ERROR_CODE_DATA_TIMEOUT, now_ms);
}

void SensorService_UpdateYaw(float yaw, uint32_t now_ms)
{
    sensor_snapshot.yaw.value = yaw;
    sensor_snapshot.yaw.valid = 1U;
    sensor_snapshot.yaw.updated_ms = now_ms;
    timeout_reported[SENSOR_SOURCE_YAW] = 0U;
    ErrorManager_Clear(ERR_MODULE_MPU6050, ERROR_CODE_DATA_TIMEOUT, now_ms);
}

void SensorService_UpdateWheelSpeeds(float left, float right, uint32_t now_ms)
{
    sensor_snapshot.speed.left = left;
    sensor_snapshot.speed.right = right;
    sensor_snapshot.speed.valid = 1U;
    sensor_snapshot.speed.updated_ms = now_ms;
}

void SensorService_UpdateColor(SensorColor color, uint32_t now_ms)
{
    sensor_snapshot.color.value = color;
    sensor_snapshot.color.valid = 1U;
    sensor_snapshot.color.updated_ms = now_ms;
    timeout_reported[SENSOR_SOURCE_COLOR] = 0U;
    ErrorManager_Clear(ERR_MODULE_COLOR, ERROR_CODE_DATA_TIMEOUT, now_ms);
}

void SensorService_UpdateDistance(float distance_cm, uint32_t now_ms)
{
    sensor_snapshot.distance_cm.value = distance_cm;
    sensor_snapshot.distance_cm.valid = 1U;
    sensor_snapshot.distance_cm.updated_ms = now_ms;
    timeout_reported[SENSOR_SOURCE_ULTRASONIC] = 0U;
    ErrorManager_Clear(ERR_MODULE_ULTRASONIC, ERROR_CODE_DATA_TIMEOUT, now_ms);
}

void SensorService_UpdateBattery(float battery_v, uint32_t now_ms)
{
    sensor_snapshot.battery_v.value = battery_v;
    sensor_snapshot.battery_v.valid = 1U;
    sensor_snapshot.battery_v.updated_ms = now_ms;
}

const SensorSnapshot *SensorService_GetSnapshot(void)
{
    return &sensor_snapshot;
}

void SensorService_CheckTimeouts(uint32_t now_ms, uint8_t mission_requires_yaw)
{
    if ((source_enabled[SENSOR_SOURCE_LINE] != 0U) &&
        (((sensor_snapshot.line.valid == 0U) &&
          (now_ms > SENSOR_LINE_TIMEOUT_MS)) ||
         ((sensor_snapshot.line.valid != 0U) &&
          ((uint32_t)(now_ms - sensor_snapshot.line.updated_ms) >
           SENSOR_LINE_TIMEOUT_MS))))
    {
        SensorService_ReportTimeoutOnce(SENSOR_SOURCE_LINE,
                                        ERR_MODULE_LINE_SENSOR,
                                        ERROR_LEVEL_FATAL,
                                        now_ms,
                                        0U);
    }

    if ((mission_requires_yaw != 0U) &&
        (source_enabled[SENSOR_SOURCE_YAW] != 0U) &&
        ((sensor_snapshot.yaw.valid == 0U) ||
         ((uint32_t)(now_ms - sensor_snapshot.yaw.updated_ms) > SENSOR_YAW_TIMEOUT_MS)))
    {
        SensorService_ReportTimeoutOnce(SENSOR_SOURCE_YAW,
                                        ERR_MODULE_MPU6050,
                                        ERROR_LEVEL_FATAL,
                                        now_ms,
                                        0U);
    }

    if ((source_enabled[SENSOR_SOURCE_COLOR] != 0U) &&
        (((sensor_snapshot.color.valid == 0U) &&
          (now_ms > SENSOR_COLOR_TIMEOUT_MS)) ||
         ((sensor_snapshot.color.valid != 0U) &&
          ((uint32_t)(now_ms - sensor_snapshot.color.updated_ms) >
           SENSOR_COLOR_TIMEOUT_MS))))
    {
        SensorService_ReportTimeoutOnce(SENSOR_SOURCE_COLOR,
                                        ERR_MODULE_COLOR,
                                        ERROR_LEVEL_WARNING,
                                        now_ms,
                                        1U);
    }

    if ((source_enabled[SENSOR_SOURCE_ULTRASONIC] != 0U) &&
        (((sensor_snapshot.distance_cm.valid == 0U) &&
          (now_ms > SENSOR_ULTRASONIC_TIMEOUT_MS)) ||
         ((uint32_t)(now_ms - sensor_snapshot.distance_cm.updated_ms) >
         SENSOR_ULTRASONIC_TIMEOUT_MS)))
    {
        SensorService_ReportTimeoutOnce(SENSOR_SOURCE_ULTRASONIC,
                                        ERR_MODULE_ULTRASONIC,
                                        ERROR_LEVEL_WARNING,
                                        now_ms,
                                        1U);
    }
}
