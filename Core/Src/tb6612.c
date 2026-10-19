#include "tb6612.h"

void TB6612_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_LEFT);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_RIGHT);
}

void TB6612_SetSpeed(uint16_t motor_side,
                     uint16_t direction,
                     uint16_t speed)
{
    if (speed >= 7200)
    {
        speed = 7199;
    }

    if (motor_side == MOTOR_LEFT)
    {
        if (direction == 0)
        {
            HAL_GPIO_WritePin(LIN1_GPIO_Port, LIN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LIN2_GPIO_Port, LIN2_Pin, GPIO_PIN_RESET);
        }
        else
        {
            HAL_GPIO_WritePin(LIN1_GPIO_Port, LIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LIN2_GPIO_Port, LIN2_Pin, GPIO_PIN_SET);
        }

        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_LEFT, speed);
    }
    else if (motor_side == MOTOR_RIGHT)
    {
        if (direction == 0)
        {
            HAL_GPIO_WritePin(RIN1_GPIO_Port, RIN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(RIN2_GPIO_Port, RIN2_Pin, GPIO_PIN_RESET);
        }
        else
        {
            HAL_GPIO_WritePin(RIN1_GPIO_Port, RIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(RIN2_GPIO_Port, RIN2_Pin, GPIO_PIN_SET);
        }

        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_RIGHT, speed);
    }
}
