#include "app/command_parser.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

static uint8_t normalize(const char *input, char *output, size_t capacity)
{
    size_t length = 0U;
    if ((input == NULL) || (output == NULL) || (capacity == 0U)) return 0U;
    while ((*input != '\0') && (length + 1U < capacity))
    {
        if (!isspace((unsigned char)*input))
        {
            output[length++] = (char)toupper((unsigned char)*input);
        }
        input++;
    }
    output[length] = '\0';
    return (length != 0U) ? 1U : 0U;
}

uint8_t CommandParser_Parse(const char *text, AppCommand *out)
{
    char command[32];
    if ((out == NULL) || (normalize(text, command, sizeof(command)) == 0U)) return 0U;
    out->type = APP_COMMAND_NONE;
    out->mode = APP_MODE_NONE;

    if (strcmp(command, "MODE=NORMAL") == 0) out->mode = APP_MODE_NORMAL;
    else if (strcmp(command, "MODE=CIRCLE") == 0) out->mode = APP_MODE_CIRCLE;
    else if (strcmp(command, "MODE=EIGHT") == 0) out->mode = APP_MODE_EIGHT;
    else if (strcmp(command, "MODE=STABLE") == 0) out->mode = APP_MODE_STABLE;
    else if (strcmp(command, "MODE=CHALLENGE") == 0) out->mode = APP_MODE_CHALLENGE;
    else if (strcmp(command, "START") == 0) out->type = APP_COMMAND_START;
    else if (strcmp(command, "STOP") == 0) out->type = APP_COMMAND_STOP;
    else if (strcmp(command, "STATUS?") == 0) out->type = APP_COMMAND_STATUS;
    else if (strcmp(command, "ERRORS?") == 0) out->type = APP_COMMAND_ERRORS;
    else if (strcmp(command, "ACK") == 0) out->type = APP_COMMAND_ACK;
    else return 0U;

    if (out->mode != APP_MODE_NONE) out->type = APP_COMMAND_SET_MODE;
    return 1U;
}
