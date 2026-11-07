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

static void test_legacy_pid_uses_weighted_position_controller(void)
{
    Line_Following_PID_Reset();
    Line_Following_PID_Prime(3500U, 3500U);
    Line_Following_PID(150U, 3500U);

    TEST_ASSERT_TRUE(Line_Following_PID_GetOutput() > 0);
    TEST_ASSERT_TRUE(captured_motion.left_target > captured_motion.right_target);
}

static void test_legacy_hybrid_uses_outer_probe_immediately(void)
{
    fake_outer_right = GPIO_PIN_SET;
    fake_outer_left = GPIO_PIN_RESET;

    Line_Following_PID_Reset();
    Line_Following_Hybrid(150U, 150U);
    TEST_ASSERT_EQ_INT('N', Line_Following_Get_Mode());
    TEST_ASSERT_TRUE(Line_Following_PID_GetOutput() > 0);
    TEST_ASSERT_TRUE(captured_motion.left_target > captured_motion.right_target);
    TEST_ASSERT_TRUE(captured_motion.left_target != CAR_LINE_RESCUE_OUTER_SPEED);
}

int main(void)
{
    test_legacy_pid_uses_weighted_position_controller();
    test_legacy_hybrid_uses_outer_probe_immediately();
    return test_failures;
}
