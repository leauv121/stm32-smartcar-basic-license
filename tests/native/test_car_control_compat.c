#include <stdint.h>

/* 为旧兼容接口提供最小 HAL 边界，测试仍直接执行真实 car_control.c。 */
typedef enum
{
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;
typedef void GPIO_TypeDef;

#define __MAIN_H
#define LINE1_GPIO_Port ((GPIO_TypeDef *)1)
#define LINE1_Pin       6U
#define LINE4_GPIO_Port ((GPIO_TypeDef *)2)
#define LINE4_Pin       1U

#include "control/motion_control.h"
#include "services/safety_service.h"
#include "test_assert.h"

static GPIO_PinState fake_outer_right;
static GPIO_PinState fake_outer_left;
static MotionRequest captured_motion;

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    (void)pin;
    return (port == LINE1_GPIO_Port) ? fake_outer_right : fake_outer_left;
}

MotionRequest SafetyService_Resolve(MotionRequest requested,
                                    uint8_t user_stop,
                                    uint8_t mission_complete)
{
    (void)user_stop;
    (void)mission_complete;
    return requested;
}

void MotionControl_Apply(MotionRequest request)
{
    captured_motion = request;
}

/* 直接编译兼容层源码，防止它与新控制器再次分叉。 */
#include "../../Core/Src/car_control.c"

static void test_legacy_pid_uses_filtered_controller(void)
{
    Line_Following_PID_Reset();
    Line_Following_PID_Prime(2000U, 2000U);
    Line_Following_PID(2000U, 2200U);

    TEST_ASSERT_EQ_INT(250, Line_Following_PID_GetOutput());
    TEST_ASSERT_EQ_INT(2110, captured_motion.left_target);
    TEST_ASSERT_EQ_INT(1610, captured_motion.right_target);
}

static void test_legacy_hybrid_uses_weak_rescue(void)
{
    fake_outer_right = GPIO_PIN_SET;
    fake_outer_left = GPIO_PIN_RESET;

    Line_Following_PID_Reset();
    Line_Following_Hybrid(2000U, 2000U);

    TEST_ASSERT_EQ_INT('R', Line_Following_Get_Mode());
    TEST_ASSERT_EQ_INT(4500, captured_motion.left_target);
    TEST_ASSERT_EQ_INT(1400, captured_motion.right_target);
}

int main(void)
{
    test_legacy_pid_uses_filtered_controller();
    test_legacy_hybrid_uses_weak_rescue();
    return test_failures;
}
