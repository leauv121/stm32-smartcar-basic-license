#ifndef KEY_H
#define KEY_H

#include "main.h"
#include "gpio.h"

uint16_t Key_Proc(void);
uint16_t Key_ProcAt(uint32_t now_ms);

#endif
