#ifndef FINAL_WORK_INDICATOR_SERVICE_H
#define FINAL_WORK_INDICATOR_SERVICE_H

#include <stdint.h>

typedef enum
{
    INDICATOR_OFF = 0,
    INDICATOR_STARTUP_FLASH_3,
    INDICATOR_TURN_LEFT,
    INDICATOR_TURN_RIGHT,
    INDICATOR_HAZARD
} IndicatorPattern;

typedef struct
{
    uint8_t left_on;
    uint8_t right_on;
    uint8_t core_led_on;
    uint8_t active;
    uint8_t completed;
} IndicatorOutput;

void IndicatorService_Init(void);
void IndicatorService_Start(IndicatorPattern pattern, uint32_t now_ms);
IndicatorOutput IndicatorService_Update(uint32_t now_ms);
IndicatorPattern IndicatorService_GetPattern(void);

#endif
