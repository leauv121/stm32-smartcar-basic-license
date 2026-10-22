#include "key.h"
#include "services/driver_algorithms.h"

#define KEY_DEBOUNCE_MS 20U

static KeyDebounceState key_debounce;
static uint8_t key_debounce_initialized;

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
    return Key_ProcAt(HAL_GetTick());
}

uint16_t Key_ProcAt(uint32_t now_ms)
{
    key_rval = Key_Scan();
    if (key_debounce_initialized == 0U)
    {
        DriverAlgorithms_KeyInit(&key_debounce, key_rval, now_ms);
        key_debounce_initialized = 1U;
        key_old = key_rval;
        return 0U;
    }

    key_down = DriverAlgorithms_KeyUpdate(&key_debounce, key_rval,
                                           now_ms, KEY_DEBOUNCE_MS);
    key_up = (uint8_t)((key_old != 0U) && (key_debounce.stable_level == 0U));
    key_old = key_debounce.stable_level;
    return (uint16_t)key_down;
}
