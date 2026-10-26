#include "services/display_service.h"

#include <stdio.h>
#include <string.h>

static const char *StateText(AppState state)
{
    switch (state)
    {
        case APP_STATE_SELF_TEST: return "TEST";
        case APP_STATE_WAIT: return "WAIT";
        case APP_STATE_RUNNING: return "RUN";
        case APP_STATE_FAULT: return "FAULT";
        default: return "?";
    }
}

static const char *ModeText(AppMode mode)
{
    switch (mode)
    {
        case APP_MODE_NORMAL: return "NORMAL";
        case APP_MODE_CIRCLE: return "CIRCLE";
        case APP_MODE_EIGHT: return "EIGHT";
        case APP_MODE_STABLE: return "STABLE";
        case APP_MODE_CHALLENGE: return "CHALLENGE";
        default: return "NONE";
    }
}

static void FixedLine(char out[17], const char *source)
{
    size_t length = strlen(source);
    size_t index;
    if (length > 16U)
    {
        length = 16U;
    }
    (void)memcpy(out, source, length);
    for (index = length; index < 16U; index++)
    {
        out[index] = ' ';
    }
    out[16] = '\0';
}

void DisplayService_FormatNormal(const DisplayModel *model, DisplayLines *out)
{
    char buffer[40];
    if ((model == 0) || (out == 0))
    {
        return;
    }

    (void)snprintf(buffer, sizeof(buffer), "%s %s",
                   StateText(model->state), ModeText(model->mode));
    FixedLine(out->line[0], buffer);
    (void)snprintf(buffer, sizeof(buffer), "L:%.1f R:%.1f R%uL%u",
                   model->left_speed, model->right_speed,
                   (unsigned int)model->outer_right,
                   (unsigned int)model->outer_left);
    FixedLine(out->line[1], buffer);
    (void)snprintf(buffer, sizeof(buffer), "T:%d/%d %c",
                   (int)model->line_target_left, (int)model->line_target_right,
                   model->line_mode);
    FixedLine(out->line[2], buffer);
    (void)snprintf(buffer, sizeof(buffer), "Y:%.1f B:%.1fV",
                   model->yaw, model->battery_v);
    FixedLine(out->line[3], buffer);
}
