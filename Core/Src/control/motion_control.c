#include "control/motion_control.h"

#include <stdint.h>

#include "config/car_parameters.h"
#include "tb6612.h"

static void apply_motor(uint16_t side, int16_t target)
{
    uint16_t direction = 0U;
    int32_t magnitude = target;

    if (magnitude < 0)
    {
        direction = 1U;
        magnitude = -magnitude;
    }
    if (magnitude > CAR_PWM_MAX) magnitude = CAR_PWM_MAX;
    TB6612_SetSpeed(side, direction, (uint16_t)magnitude);
}

void MotionControl_Apply(MotionRequest request)
{
    if (request.mode == MOTION_STOP)
    {
        apply_motor(MOTOR_LEFT, 0);
        apply_motor(MOTOR_RIGHT, 0);
        return;
    }
    apply_motor(MOTOR_LEFT, request.left_target);
    apply_motor(MOTOR_RIGHT, request.right_target);
}
