#ifndef FINAL_WORK_SENSOR_SERVICE_H
#define FINAL_WORK_SENSOR_SERVICE_H

#include <stdint.h>

typedef enum
{
    SENSOR_SOURCE_LINE = 0,
    SENSOR_SOURCE_YAW,
    SENSOR_SOURCE_SPEED,
    SENSOR_SOURCE_COLOR,
    SENSOR_SOURCE_ULTRASONIC,
    SENSOR_SOURCE_BATTERY,
    SENSOR_SOURCE_COUNT
} SensorSource;

typedef enum
{
    SENSOR_COLOR_UNKNOWN = 0,
    SENSOR_COLOR_RED,
    SENSOR_COLOR_GREEN,
    SENSOR_COLOR_OTHER
} SensorColor;

typedef struct
{
    uint16_t left_adc;
    uint16_t right_adc;
    uint8_t outer_right;
    uint8_t outer_left;
    uint8_t valid;
    uint32_t updated_ms;
} LineSensorSample;

typedef struct
{
    float value;
    uint8_t valid;
    uint32_t updated_ms;
} ScalarSensorSample;

typedef struct
{
    float left;
    float right;
    uint8_t valid;
    uint32_t updated_ms;
} WheelSpeedSample;

typedef struct
{
    SensorColor value;
    uint8_t valid;
    uint32_t updated_ms;
} ColorSensorSample;

typedef struct
{
    LineSensorSample line;
    ScalarSensorSample yaw;
    WheelSpeedSample speed;
    ColorSensorSample color;
    ScalarSensorSample distance_cm;
    ScalarSensorSample battery_v;
} SensorSnapshot;

void SensorService_Init(void);
void SensorService_SetSourceEnabled(SensorSource source, uint8_t enabled);
void SensorService_UpdateLine(uint16_t left_adc,
                              uint16_t right_adc,
                              uint8_t outer_right,
                              uint8_t outer_left,
                              uint32_t now_ms);
void SensorService_UpdateYaw(float yaw, uint32_t now_ms);
void SensorService_UpdateWheelSpeeds(float left, float right, uint32_t now_ms);
void SensorService_UpdateColor(SensorColor color, uint32_t now_ms);
void SensorService_UpdateDistance(float distance_cm, uint32_t now_ms);
void SensorService_UpdateBattery(float battery_v, uint32_t now_ms);
const SensorSnapshot *SensorService_GetSnapshot(void);
void SensorService_CheckTimeouts(uint32_t now_ms, uint8_t mission_requires_yaw);

#endif
