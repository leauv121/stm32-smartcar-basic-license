#ifndef __SPEED_MEASURE_H
#define __SPEED_MEASURE_H

#include "main.h"

void Speed_MeasureInit(void);

uint8_t Speed_Measure_GetLeftDataState(void);
float Speed_Measure_GetLeftData(void);
void Speed_Measure_LeftDataProcessComplt(void);

uint8_t Speed_Measure_GetRightDataState(void);
float Speed_Measure_GetRightData(void);
void Speed_Measure_RightDataProcessComplt(void);

void Speed_MeasureTimOverflowHandler(void);
void Speed_MeasureTimIcHandler(uint32_t channel);

#endif

