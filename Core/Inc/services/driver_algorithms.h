#ifndef FINAL_WORK_DRIVER_ALGORITHMS_H
#define FINAL_WORK_DRIVER_ALGORITHMS_H

#include <stdint.h>

typedef enum
{
    DRIVER_COLOR_NONE = 0,
    DRIVER_COLOR_RED,
    DRIVER_COLOR_GREEN
} DriverColor;

typedef struct
{
    DriverColor candidate;
    DriverColor emitted;
    uint8_t consecutive_count;
} ColorConfirmState;

typedef struct
{
    uint8_t stable_level;
    uint8_t candidate_level;
    uint32_t candidate_since_ms;
} KeyDebounceState;

float DriverAlgorithms_UltrasonicCm(uint32_t pulse_us);
float DriverAlgorithms_SpeedRpm(uint32_t revolution_ticks, uint32_t timer_hz);
uint8_t DriverAlgorithms_IsTimedOut(uint32_t sample_ms,
                                    uint32_t now_ms,
                                    uint32_t timeout_ms);
DriverColor DriverAlgorithms_ConfirmColor(ColorConfirmState *state,
                                          DriverColor sample,
                                          uint8_t required_count);
void DriverAlgorithms_KeyInit(KeyDebounceState *state,
                              uint8_t initial_level,
                              uint32_t now_ms);
uint8_t DriverAlgorithms_KeyUpdate(KeyDebounceState *state,
                                   uint8_t raw_level,
                                   uint32_t now_ms,
                                   uint32_t debounce_ms);

#endif
