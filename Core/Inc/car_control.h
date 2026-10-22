#ifndef __CAR_CONTROL_H
#define __CAR_CONTROL_H

#include "main.h"

void Smart_Car_Turn_Left(void);
void Smart_Car_Turn_Left_Weak(void);
void Smart_Car_Turn_Right(void);
void Smart_Car_Turn_Right_Weak(void);
void Smart_Car_Go_Straight(void);
void Smart_Car_Go_Straight_Fast(void);
void Smart_Car_Stop(void);
void Line_Following_PID(uint16_t left_adc, uint16_t right_adc);
void Line_Following_PID_Reset(void);
int32_t Line_Following_PID_GetOutput(void);

#endif
