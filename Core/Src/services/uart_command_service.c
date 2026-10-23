#include "services/uart_command_service.h"

#include <stdio.h>
#include <string.h>

#include "app/command_parser.h"
#include "services/error_manager.h"

#define UART_COMMAND_BUFFER_SIZE 48U
#define UART_COMMAND_QUEUE_DEPTH  3U
#define UART_COMMAND_QUEUE_LIMIT  2U

static UART_HandleTypeDef *command_uart;
static uint8_t receive_byte;
static char receive_queue[UART_COMMAND_QUEUE_DEPTH][UART_COMMAND_BUFFER_SIZE];
static volatile uint8_t receive_length;
static volatile uint8_t write_index;
static volatile uint8_t read_index;
static volatile uint8_t queued_count;
static volatile uint8_t overflow_pending;

void UartCommandService_Init(UART_HandleTypeDef *uart)
{
    command_uart = uart;
    receive_length = 0U;
    write_index = 0U;
    read_index = 0U;
    queued_count = 0U;
    overflow_pending = 0U;
    if (command_uart != 0)
    {
        (void)HAL_UART_Receive_IT(command_uart, &receive_byte, 1U);
    }
}

void UartCommandService_OnReceive(UART_HandleTypeDef *uart)
{
    if ((uart == command_uart) && (command_uart != 0))
    {
        if ((receive_byte == '\r') || (receive_byte == '\n'))
        {
            if (receive_length > 0U)
            {
                if (queued_count < UART_COMMAND_QUEUE_LIMIT)
                {
                    receive_queue[write_index][receive_length] = '\0';
                    queued_count++;
                    write_index = (uint8_t)((write_index + 1U) %
                                            UART_COMMAND_QUEUE_DEPTH);
                }
                else
                {
                    overflow_pending = 1U;
                }
                receive_length = 0U;
            }
        }
        else if (queued_count < UART_COMMAND_QUEUE_LIMIT)
        {
            if (receive_length < (UART_COMMAND_BUFFER_SIZE - 1U))
            {
                receive_queue[write_index][receive_length++] = (char)receive_byte;
            }
            else
            {
                receive_length = 0U;
                /* 中断只记录标志，错误表由主循环更新。 */
                overflow_pending = 1U;
            }
        }
        else
        {
            overflow_pending = 1U;
        }
        (void)HAL_UART_Receive_IT(command_uart, &receive_byte, 1U);
    }
}

void UartCommandService_OnError(UART_HandleTypeDef *uart)
{
    if ((uart == command_uart) && (command_uart != 0))
    {
        receive_length = 0U;
        overflow_pending = 1U;
        if (uart->RxState == HAL_UART_STATE_READY)
        {
            (void)HAL_UART_Receive_IT(command_uart, &receive_byte, 1U);
        }
    }
}

uint8_t UartCommandService_Poll(AppCommand *command)
{
    char local[UART_COMMAND_BUFFER_SIZE];
    uint8_t length;
    uint8_t parsed;

    if (overflow_pending != 0U)
    {
        overflow_pending = 0U;
        ErrorManager_Report(ERR_MODULE_SYSTEM, ERROR_CODE_COMMUNICATION,
                            ERROR_LEVEL_WARNING, HAL_GetTick(), 1U);
    }
    if ((command == 0) || (queued_count == 0U))
    {
        return 0U;
    }

    __disable_irq();
    length = (uint8_t)strlen(receive_queue[read_index]);
    (void)memcpy(local, receive_queue[read_index], (size_t)length + 1U);
    read_index = (uint8_t)((read_index + 1U) % UART_COMMAND_QUEUE_DEPTH);
    queued_count--;
    __enable_irq();

    parsed = CommandParser_Parse(local, command);
    if (parsed == 0U)
    {
        UartCommandService_SendText("ERR UNKNOWN COMMAND\r\n");
    }
    return parsed;
}

void UartCommandService_SendText(const char *text)
{
    if ((command_uart != 0) && (text != 0))
    {
        (void)HAL_UART_Transmit(command_uart, (uint8_t *)text,
                               (uint16_t)strlen(text), 100U);
    }
}

void UartCommandService_SendResult(const AppOutput *output)
{
    char text[96];
    const ErrorRecord *error;
    if (output == 0)
    {
        return;
    }
    (void)snprintf(text, sizeof(text),
                   "STATE=%u MODE=%u RESULT=%u ERRORS=%u\r\n",
                   (unsigned int)output->state,
                   (unsigned int)output->mode,
                   (unsigned int)output->command_result,
                   (unsigned int)ErrorManager_GetActiveCount());
    UartCommandService_SendText(text);
    if (output->errors_requested != 0U)
    {
        error = ErrorManager_GetHighest();
        if (error == 0)
        {
            UartCommandService_SendText("ERROR=NONE\r\n");
        }
        else
        {
            (void)snprintf(text, sizeof(text),
                           "ERROR MOD=%u CODE=%u LEVEL=%u COUNT=%u\r\n",
                           (unsigned int)error->module,
                           (unsigned int)error->code,
                           (unsigned int)error->level,
                           (unsigned int)error->occurrence_count);
            UartCommandService_SendText(text);
        }
    }
}
