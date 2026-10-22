#ifndef FINAL_WORK_LINE_FOLLOW_CONTROL_H
#define FINAL_WORK_LINE_FOLLOW_CONTROL_H

#include <stdint.h>

typedef enum
{
    LINE_FOLLOW_PID = 0,
    LINE_FOLLOW_RESCUE_LEFT,
    LINE_FOLLOW_RESCUE_RIGHT
} LineFollowMode;

typedef struct
{
    uint16_t left_adc;
    uint16_t right_adc;
    uint8_t outer_right;
    uint8_t outer_left;
} LineFollowInput;

typedef struct
{
    int16_t left_target;
    int16_t right_target;
    int32_t pid_output;
    LineFollowMode mode;
} LineFollowOutput;

void LineFollowControl_Reset(void);
void LineFollowControl_Prime(uint16_t left_adc, uint16_t right_adc);
LineFollowOutput LineFollowControl_Update(LineFollowInput input);

#endif
