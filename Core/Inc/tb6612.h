#ifndef TB6612_H
#define TB6612_H

#include "main.h"
#include "tim.h"
#include "gpio.h"

#define TIM_CHANNEL_LEFT   TIM_CHANNEL_1
#define TIM_CHANNEL_RIGHT  TIM_CHANNEL_2

#define MOTOR_LEFT   0
#define MOTOR_RIGHT  1

void TB6612_Init(void);
void TB6612_SetSpeed(uint16_t motor_side,
                     uint16_t direction,
                     uint16_t speed);

#endif
