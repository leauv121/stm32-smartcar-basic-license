#include "speed_measure.h"
#include "tim.h"

#define Speed_Measure_TIM_Handler    htim4
#define Speed_Measure_LEFT_CHANNEL   TIM_CHANNEL_4
#define Speed_Measure_RIGHT_CHANNEL  TIM_CHANNEL_3
#define Speed_Measure_TIM_Period     10000U
#define Speed_Measure_SAMPLE_COUNT   20U

typedef struct
{
    uint32_t measure_buffer[Speed_Measure_SAMPLE_COUNT];
    uint32_t measure_interval_buffer[2];
    uint8_t measure_interval_flag;
    uint8_t measure_count;
    uint16_t measure_overflowcount;
    volatile uint8_t measure_complt_flag;
    float motor_speed;
} Speed_MeasureState;

static Speed_MeasureState left_measure;
static Speed_MeasureState right_measure;

static void Speed_Measure_CalculateData(Speed_MeasureState *state)
{
    uint8_t i;
    uint32_t data_count = 0U;

    /* The encoder disk has 20 holes: 20 pulse intervals are one revolution. */
    for (i = 0U; i < Speed_Measure_SAMPLE_COUNT; i++)
    {
        data_count += state->measure_buffer[i];
    }

    state->measure_count = 0U;
    if (data_count > 0U)
    {
        state->motor_speed = Speed_Measure_TIM_Period / (float)data_count;
    }
    else
    {
        state->motor_speed = 0.0f;
    }
    state->measure_complt_flag = 1U;
}

void Speed_MeasureInit(void)
{
    __HAL_TIM_CLEAR_IT(&Speed_Measure_TIM_Handler, TIM_IT_CC3);
    __HAL_TIM_CLEAR_IT(&Speed_Measure_TIM_Handler, TIM_IT_CC4);
    __HAL_TIM_ENABLE_IT(&Speed_Measure_TIM_Handler, TIM_IT_UPDATE);

    HAL_TIM_IC_Start_IT(&Speed_Measure_TIM_Handler, Speed_Measure_RIGHT_CHANNEL);
    HAL_TIM_IC_Start_IT(&Speed_Measure_TIM_Handler, Speed_Measure_LEFT_CHANNEL);
}

uint8_t Speed_Measure_GetLeftDataState(void)
{
    return left_measure.measure_complt_flag;
}

float Speed_Measure_GetLeftData(void)
{
    return left_measure.motor_speed;
}

void Speed_Measure_LeftDataProcessComplt(void)
{
    left_measure.measure_complt_flag = 0U;
}

uint8_t Speed_Measure_GetRightDataState(void)
{
    return right_measure.measure_complt_flag;
}

float Speed_Measure_GetRightData(void)
{
    return right_measure.motor_speed;
}

void Speed_Measure_RightDataProcessComplt(void)
{
    right_measure.measure_complt_flag = 0U;
}

void Speed_MeasureTimOverflowHandler(void)
{
    if ((left_measure.measure_interval_flag != 0U) &&
        (left_measure.measure_complt_flag == 0U))
    {
        left_measure.measure_overflowcount++;
    }

    if ((right_measure.measure_interval_flag != 0U) &&
        (right_measure.measure_complt_flag == 0U))
    {
        right_measure.measure_overflowcount++;
    }
}

void Speed_MeasureTimIcHandler(uint32_t channel)
{
    Speed_MeasureState *state;

    if (channel == Speed_Measure_LEFT_CHANNEL)
    {
        state = &left_measure;
    }
    else if (channel == Speed_Measure_RIGHT_CHANNEL)
    {
        state = &right_measure;
    }
    else
    {
        return;
    }

    if (state->measure_complt_flag != 0U)
    {
        return;
    }

    if (state->measure_interval_flag == 0U)
    {
        state->measure_interval_buffer[0] =
            HAL_TIM_ReadCapturedValue(&Speed_Measure_TIM_Handler, channel);
        state->measure_overflowcount = 0U;
        state->measure_interval_flag = 1U;
    }
    else
    {
        state->measure_interval_buffer[1] =
            HAL_TIM_ReadCapturedValue(&Speed_Measure_TIM_Handler, channel);
        state->measure_interval_flag = 0U;

        state->measure_buffer[state->measure_count] =
            state->measure_interval_buffer[1] -
            state->measure_interval_buffer[0] +
            Speed_Measure_TIM_Handler.Init.Period *
            state->measure_overflowcount;

        state->measure_count++;
        state->measure_overflowcount = 0U;

        if (state->measure_count >= Speed_Measure_SAMPLE_COUNT)
        {
            Speed_Measure_CalculateData(state);
        }
    }
}
