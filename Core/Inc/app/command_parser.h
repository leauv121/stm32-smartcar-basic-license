#ifndef FINAL_WORK_COMMAND_PARSER_H
#define FINAL_WORK_COMMAND_PARSER_H

#include <stdint.h>

#include "app/app_types.h"

typedef enum
{
    APP_COMMAND_NONE = 0,
    APP_COMMAND_SET_MODE,
    APP_COMMAND_START,
    APP_COMMAND_STOP,
    APP_COMMAND_STATUS,
    APP_COMMAND_ERRORS,
    APP_COMMAND_ACK
} AppCommandType;

typedef struct
{
    AppCommandType type;
    AppMode mode;
} AppCommand;

uint8_t CommandParser_Parse(const char *text, AppCommand *out);

#endif
