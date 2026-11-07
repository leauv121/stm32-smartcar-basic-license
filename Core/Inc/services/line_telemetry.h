#ifndef FINAL_WORK_LINE_TELEMETRY_H
#define FINAL_WORK_LINE_TELEMETRY_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint32_t now_ms;
    uint16_t left_adc;
    uint16_t right_adc;
    uint8_t outer_right;
    uint8_t outer_left;
    int32_t pid_output;
    int16_t left_target;
    int16_t right_target;
    char mode;
} LineTelemetrySample;

int LineTelemetry_Format(const LineTelemetrySample *sample,
                         char *text,
                         size_t capacity);

#endif
