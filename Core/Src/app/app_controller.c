#include "app/app_controller.h"

#include <string.h>

#include "services/error_manager.h"

static AppState app_state;
static AppMode selected_mode;

static uint8_t IsMissionMode(AppMode mode)
{
    return (uint8_t)((mode >= APP_MODE_NORMAL) && (mode <= APP_MODE_CHALLENGE));
}

static void StopMission(void)
{
    NormalMission_Stop();
    AdvancedMission_Stop();
}

static void StartMission(uint32_t now_ms)
{
    if (selected_mode == APP_MODE_NORMAL)
    {
        NormalMission_Start(now_ms);
    }
    else
    {
        AdvancedMission_Start(selected_mode, now_ms);
    }
    app_state = APP_STATE_RUNNING;
}

void AppController_Init(void)
{
    app_state = APP_STATE_SELF_TEST;
    selected_mode = APP_MODE_NONE;
    NormalMission_Init();
    AdvancedMission_Init();
}

AppState AppController_GetState(void)
{
    return app_state;
}

AppMode AppController_GetMode(void)
{
    return selected_mode;
}

static void ProcessCommand(const AppCommand *command, uint8_t stop_latched,
                           uint32_t now_ms,
                           AppOutput *output)
{
    if ((command == 0) || (command->type == APP_COMMAND_NONE))
    {
        return;
    }

    switch (command->type)
    {
        case APP_COMMAND_SET_MODE:
            if (app_state == APP_STATE_RUNNING)
            {
                output->command_result = APP_RESULT_BUSY;
            }
            else if (IsMissionMode(command->mode) == 0U)
            {
                output->command_result = APP_RESULT_INVALID_MODE;
            }
            else
            {
                selected_mode = command->mode;
                output->command_result = APP_RESULT_OK;
            }
            break;

        case APP_COMMAND_START:
            if (ErrorManager_HasFatal() != 0U)
            {
                output->command_result = APP_RESULT_FATAL_LOCKED;
            }
            else if (app_state == APP_STATE_RUNNING)
            {
                output->command_result = APP_RESULT_BUSY;
            }
            else if (stop_latched != 0U)
            {
                output->command_result = APP_RESULT_ACK_REQUIRED;
            }
            else if (selected_mode == APP_MODE_NONE)
            {
                output->command_result = APP_RESULT_MODE_REQUIRED;
            }
            else if (app_state != APP_STATE_WAIT)
            {
                output->command_result = APP_RESULT_FATAL_LOCKED;
            }
            else
            {
                StartMission(now_ms);
                output->command_result = APP_RESULT_OK;
            }
            break;

        case APP_COMMAND_STOP:
            output->user_stop_requested = (uint8_t)(app_state == APP_STATE_RUNNING);
            StopMission();
            if (app_state == APP_STATE_RUNNING)
            {
                app_state = APP_STATE_WAIT;
            }
            output->command_result = APP_RESULT_OK;
            break;

        case APP_COMMAND_STATUS:
            output->status_requested = 1U;
            output->command_result = APP_RESULT_OK;
            break;

        case APP_COMMAND_ERRORS:
            output->errors_requested = 1U;
            output->command_result = APP_RESULT_OK;
            break;

        case APP_COMMAND_ACK:
            if (ErrorManager_HasFatal() != 0U)
            {
                output->command_result = APP_RESULT_FATAL_LOCKED;
            }
            else
            {
                if (app_state == APP_STATE_FAULT)
                {
                    app_state = APP_STATE_WAIT;
                }
                output->command_result = APP_RESULT_OK;
            }
            break;

        default:
            break;
    }
}

AppOutput AppController_Update(const AppInput *input, uint32_t now_ms)
{
    AppOutput output;
    MissionOutput mission;

    (void)memset(&output, 0, sizeof(output));
    (void)memset(&mission, 0, sizeof(mission));

    output.motion = MotionRequest_Stop();
    if (input == 0)
    {
        output.state = app_state;
        output.mode = selected_mode;
        return output;
    }

    if (app_state == APP_STATE_SELF_TEST)
    {
        if (input->self_test_complete != 0U)
        {
            app_state = (input->self_test_safe != 0U) ? APP_STATE_WAIT : APP_STATE_FAULT;
        }
    }

    if (ErrorManager_HasFatal() != 0U)
    {
        StopMission();
        app_state = APP_STATE_FAULT;
    }

    ProcessCommand(&input->command, input->stop_latched, now_ms, &output);

    /* USER 键与串口 START/STOP 共用同一任务入口，便于脱机演示。 */
    if ((input->command.type == APP_COMMAND_NONE) &&
        (input->user_button_pressed != 0U))
    {
        if (app_state == APP_STATE_RUNNING)
        {
            output.user_stop_requested = 1U;
            StopMission();
            app_state = APP_STATE_WAIT;
        }
        else if ((app_state == APP_STATE_WAIT) && (selected_mode != APP_MODE_NONE) &&
                 (input->stop_latched == 0U) &&
                 (ErrorManager_HasFatal() == 0U))
        {
            StartMission(now_ms);
        }
    }

    if (app_state == APP_STATE_RUNNING)
    {
        if (selected_mode == APP_MODE_NORMAL)
        {
            mission = NormalMission_Update(&input->normal, now_ms);
        }
        else
        {
            mission = AdvancedMission_Update(&input->advanced, now_ms);
        }

        output.motion = mission.motion;
        output.indicator = mission.indicator;
        output.mission_complete = mission.complete;
        output.point_reached = mission.point_reached;
        if (mission.complete != 0U)
        {
            StopMission();
            app_state = APP_STATE_WAIT;
            output.motion = MotionRequest_Stop();
        }
    }

    output.state = app_state;
    output.mode = selected_mode;
    if (app_state == APP_STATE_SELF_TEST)
    {
        output.display_page = APP_DISPLAY_SELF_TEST;
    }
    else if (app_state == APP_STATE_RUNNING)
    {
        output.display_page = APP_DISPLAY_RUNNING;
    }
    else if (app_state == APP_STATE_FAULT)
    {
        output.display_page = APP_DISPLAY_FAULT;
    }
    else
    {
        output.display_page = APP_DISPLAY_READY;
    }

    return output;
}
