#ifndef FINAL_WORK_UART_COMMAND_SERVICE_H
#define FINAL_WORK_UART_COMMAND_SERVICE_H

#include <stdint.h>

#include "app/app_controller.h"
#include "main.h"

void UartCommandService_Init(UART_HandleTypeDef *uart);
void UartCommandService_OnReceive(UART_HandleTypeDef *uart);
void UartCommandService_OnError(UART_HandleTypeDef *uart);
uint8_t UartCommandService_Poll(AppCommand *command);
void UartCommandService_SendText(const char *text);
void UartCommandService_SendResult(const AppOutput *output);

#endif
