#include "services/diagnostic_view.h"

#include <stdio.h>
#include <string.h>

static const char *DiagnosticView_ModuleText(ErrorModule module)
{
    switch (module)
    {
        case ERR_MODULE_ADC:          return "ADC";
        case ERR_MODULE_LINE_SENSOR:  return "LINE";
        case ERR_MODULE_MPU6050:      return "MPU6050";
        case ERR_MODULE_SPEED_LEFT:   return "SPEED L";
        case ERR_MODULE_SPEED_RIGHT:  return "SPEED R";
        case ERR_MODULE_COLOR:        return "COLOR";
        case ERR_MODULE_ULTRASONIC:   return "ULTRASONIC";
        case ERR_MODULE_BATTERY:      return "BATTERY";
        case ERR_MODULE_MOTOR:        return "MOTOR";
        case ERR_MODULE_MISSION:      return "MISSION";
        case ERR_MODULE_DISPLAY:      return "DISPLAY";
        case ERR_MODULE_SYSTEM:
        default:                      return "SYSTEM";
    }
}

static const char *DiagnosticView_CodeText(ErrorCode code)
{
    switch (code)
    {
        case ERROR_CODE_NOT_FOUND:         return "NOT FOUND";
        case ERROR_CODE_INIT_FAILED:       return "INIT FAIL";
        case ERROR_CODE_DATA_TIMEOUT:      return "DATA TIME";
        case ERROR_CODE_VALUE_INVALID:     return "BAD VALUE";
        case ERROR_CODE_COMMUNICATION:     return "COMM";
        case ERROR_CODE_LOW_VOLTAGE:       return "LOW VOLT";
        case ERROR_CODE_STATE_INVALID:     return "BAD STATE";
        case ERROR_CODE_RESOURCE_CONFLICT: return "RESOURCE";
        case ERROR_CODE_NONE:
        default:                           return "UNKNOWN";
    }
}

void DiagnosticView_Format(const ErrorRecord *record,
                           uint8_t index,
                           uint8_t count,
                           DiagnosticLines *out)
{
    const char *title;
    uint8_t safe_index = (uint8_t)(index + 1U);
    uint8_t safe_count = (count == 0U) ? 1U : count;

    if (out == NULL)
    {
        return;
    }

    memset(out, 0, sizeof(*out));
    if (record == NULL)
    {
        (void)snprintf(out->line[0], sizeof(out->line[0]), "NO ERROR");
        return;
    }

    title = (record->level == ERROR_LEVEL_FATAL) ? "ERROR" : "WARNING";
    (void)snprintf(out->line[0], sizeof(out->line[0]),
                   "%s %02u/%02u", title, safe_index, safe_count);
    (void)snprintf(out->line[1], sizeof(out->line[1]),
                   "MOD:%.12s", DiagnosticView_ModuleText(record->module));
    (void)snprintf(out->line[2], sizeof(out->line[2]),
                   "CODE:%.11s", DiagnosticView_CodeText(record->code));
    (void)snprintf(out->line[3], sizeof(out->line[3]), "%s",
                   (record->level == ERROR_LEVEL_FATAL) ?
                   "MOTOR STOP" : "CHECK SENSOR");
}
