#include "services/line_telemetry.h"

#include <stdio.h>

int LineTelemetry_Format(const LineTelemetrySample *sample,
                         char *text,
                         size_t capacity)
{
    int length;

    if ((sample == 0) || (text == 0) || (capacity == 0U))
    {
        return 0;
    }

    length = snprintf(text, capacity,
                      "LF,%lu,%u,%u,%u,%u,%ld,%d,%d,%c\r\n",
                      (unsigned long)sample->now_ms,
                      (unsigned int)sample->left_adc,
                      (unsigned int)sample->right_adc,
                      (unsigned int)sample->outer_right,
                      (unsigned int)sample->outer_left,
                      (long)sample->pid_output,
                      (int)sample->left_target,
                      (int)sample->right_target,
                      sample->mode);
    if ((length <= 0) || ((size_t)length >= capacity))
    {
        text[0] = '\0';
        return 0;
    }
    return length;
}
