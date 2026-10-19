#ifndef __SOFT_I2C_H
#define __SOFT_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void SoftI2C_Init(void);
void SoftI2C_Start(void);
void SoftI2C_Stop(void);
void SoftI2C_SendByte(uint8_t Byte);

#ifdef __cplusplus
}
#endif

#endif
