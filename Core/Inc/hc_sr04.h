#ifndef HCSR04_H_
#define HCSR04_H_

#include "main.h"

typedef struct
{
    uint8_t  edge_state;
    uint16_t tim_overflow_counter;
    uint32_t prescaler;
    uint32_t period;
    uint32_t t1;	//	上升沿时间
    uint32_t t2;	//	下降沿时间
    uint32_t high_level_us;	//	高电平持续时间
    float    distance;
    TIM_HandleTypeDef* TimerHandle;
    TIM_TypeDef* instance;
    uint32_t ic_tim_ch;
    HAL_TIM_ActiveChannel active_channel;
}Hcsr04InfoTypeDef;

extern Hcsr04InfoTypeDef Hcsr04Info;

/**
 * @description: 超声波模块的输入捕获定时器通道初始化
 * @param {TIM_HandleTypeDef} *htim
 * @param {uint32_t} Channel
 * @return {*}
 */
void HC_SR04Init(TIM_HandleTypeDef *htim, uint32_t Channel);

/**
 * @description: HC-SR04触发
 * @param {*}
 * @return {*}
 */
void HC_SR04Start(void);

/**
 * @description: 定时器计数溢出中断处理函数
 * @param {*}    main.c中重定义void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
 * @return {*}
 */
void Hcsr04TimOverflowIsr(void);

/**
 * @description: 输入捕获计算高电平时间->距离
 * @param {*}    main.c中重定义void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
 * @return {*}
 */
void Hcsr04TimIcIsr(void);

/**
 * @description: 读取距离
 * @param {*}
 * @return {*}
 */
float HC_SR04Read(void);

#endif /* HCSR04_H_ */


