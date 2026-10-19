#include "key.h"

uint8_t Key_Scan(void)
{
    uint8_t key_value = 0;

    if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
    {
        key_value = 1;
    }

    return key_value;
}

extern uint8_t key_rval;

uint8_t key_up;
uint8_t key_down;
uint8_t key_old;

uint16_t Key_Proc(void)
{
    key_rval = Key_Scan();

    key_down = key_rval & (key_rval ^ key_old);
    key_up = ~key_rval & (key_rval ^ key_old);
    key_old = key_rval;

    if (key_down == 1)
    {
        return 1;
    }

    return 0;
}
