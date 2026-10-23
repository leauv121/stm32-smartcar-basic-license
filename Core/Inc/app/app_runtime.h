#ifndef FINAL_WORK_APP_RUNTIME_H
#define FINAL_WORK_APP_RUNTIME_H

#include <stdint.h>

#include "main.h"

void AppRuntime_Init(void);
void AppRuntime_RunOnce(void);
void AppRuntime_OnMpuReady(void);
void AppRuntime_OnAdcError(void);
void AppRuntime_OnTimerPeriod(TIM_HandleTypeDef *timer);
void AppRuntime_OnTimerCapture(TIM_HandleTypeDef *timer);
void AppRuntime_OnUartReceive(UART_HandleTypeDef *uart);
void AppRuntime_OnUartError(UART_HandleTypeDef *uart);

#endif
