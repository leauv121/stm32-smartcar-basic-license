#ifndef FINAL_WORK_ERROR_MANAGER_H
#define FINAL_WORK_ERROR_MANAGER_H

#include <stdint.h>

#define ERROR_MANAGER_CAPACITY             16U
#define ERROR_DISPLAY_ROTATE_INTERVAL_MS 2000U

typedef enum
{
    ERR_MODULE_SYSTEM = 0,
    ERR_MODULE_ADC,
    ERR_MODULE_LINE_SENSOR,
    ERR_MODULE_MPU6050,
    ERR_MODULE_SPEED_LEFT,
    ERR_MODULE_SPEED_RIGHT,
    ERR_MODULE_COLOR,
    ERR_MODULE_ULTRASONIC,
    ERR_MODULE_BATTERY,
    ERR_MODULE_MOTOR,
    ERR_MODULE_MISSION,
    ERR_MODULE_DISPLAY,
    ERR_MODULE_COUNT
} ErrorModule;

typedef enum
{
    ERROR_CODE_NONE = 0,
    ERROR_CODE_NOT_FOUND,
    ERROR_CODE_INIT_FAILED,
    ERROR_CODE_DATA_TIMEOUT,
    ERROR_CODE_VALUE_INVALID,
    ERROR_CODE_COMMUNICATION,
    ERROR_CODE_LOW_VOLTAGE,
    ERROR_CODE_STATE_INVALID,
    ERROR_CODE_RESOURCE_CONFLICT
} ErrorCode;

typedef enum
{
    ERROR_LEVEL_INFO = 0,
    ERROR_LEVEL_WARNING,
    ERROR_LEVEL_FATAL
} ErrorLevel;

typedef struct
{
    ErrorModule module;
    ErrorCode code;
    ErrorLevel level;
    uint32_t first_seen_ms;
    uint32_t last_seen_ms;
    uint32_t cleared_ms;
    uint16_t occurrence_count;
    uint8_t active;
    uint8_t auto_recover;
} ErrorRecord;

void ErrorManager_Init(void);
void ErrorManager_Report(ErrorModule module,
                         ErrorCode code,
                         ErrorLevel level,
                         uint32_t now_ms,
                         uint8_t auto_recover);
void ErrorManager_Clear(ErrorModule module, ErrorCode code, uint32_t now_ms);
uint8_t ErrorManager_GetActiveCount(void);
uint8_t ErrorManager_HasFatal(void);
const ErrorRecord *ErrorManager_GetHighest(void);
const ErrorRecord *ErrorManager_GetDisplayRecord(uint32_t now_ms);

#endif
