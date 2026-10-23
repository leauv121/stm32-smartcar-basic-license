#ifndef FINAL_WORK_APP_CONTROLLER_H
#define FINAL_WORK_APP_CONTROLLER_H

#include <stdint.h>

#include "app/advanced_mission.h"
#include "app/command_parser.h"
#include "app/normal_mission.h"

typedef enum
{
    APP_STATE_SELF_TEST = 0,
    APP_STATE_WAIT,
    APP_STATE_RUNNING,
    APP_STATE_FAULT
} AppState;

typedef enum
{
    APP_RESULT_NONE = 0,
    APP_RESULT_OK,
    APP_RESULT_BUSY,
    APP_RESULT_MODE_REQUIRED,
    APP_RESULT_FATAL_LOCKED,
    APP_RESULT_ACK_REQUIRED,
    APP_RESULT_INVALID_MODE
} AppCommandResult;

typedef enum
{
    APP_DISPLAY_SELF_TEST = 0,
    APP_DISPLAY_READY,
    APP_DISPLAY_RUNNING,
    APP_DISPLAY_FAULT
} AppDisplayPage;

typedef struct
{
    uint8_t self_test_complete;
    uint8_t self_test_safe;
    uint8_t user_button_pressed;
    uint8_t stop_latched;
    AppCommand command;
    NormalMissionInput normal;
    AdvancedMissionInput advanced;
} AppInput;

typedef struct
{
    AppState state;
    AppMode mode;
    MotionRequest motion;
    IndicatorPattern indicator;
    AppDisplayPage display_page;
    AppCommandResult command_result;
    uint8_t status_requested;
    uint8_t errors_requested;
    uint8_t mission_complete;
    uint8_t point_reached;
    uint8_t user_stop_requested;
} AppOutput;

void AppController_Init(void);
AppOutput AppController_Update(const AppInput *input, uint32_t now_ms);
AppState AppController_GetState(void);
AppMode AppController_GetMode(void);

#endif
