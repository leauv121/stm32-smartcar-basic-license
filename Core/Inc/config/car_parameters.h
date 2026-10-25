#ifndef FINAL_WORK_CAR_PARAMETERS_H
#define FINAL_WORK_CAR_PARAMETERS_H

/* 基础调度周期，单位均为毫秒。 */
#define CAR_CONTROL_INTERVAL_MS             20U
#define CAR_DISPLAY_INTERVAL_MS            200U
#define CAR_UART_INTERVAL_MS               200U
#define CAR_DIAGNOSTIC_ROTATE_INTERVAL_MS 2000U

/* TIM2 自动重装值为 7199，因此有效 PWM 范围为 0～7199。 */
#define CAR_PWM_MAX                       7199

/* 当前实车循迹基线参数，后续只在本文件集中标定。 */
#define CAR_LINE_PID_KP                   1.0f
#define CAR_LINE_PID_KI                   0.0f
#define CAR_LINE_PID_KD                   0.5f
#define CAR_LINE_PID_START_SPEED          1800
#define CAR_LINE_PID_BASE_SPEED           3700
#define CAR_LINE_PID_MAX_SPEED            6500
#define CAR_LINE_PID_INTEGRAL_LIMIT       8000
#define CAR_LINE_PID_ERROR_DEADBAND         60
#define CAR_LINE_PID_SOFT_START_STEP        60

/* 外侧数字传感器触发急弯救线时，保持双轮前进以减少模式切换顿挫。 */
#define CAR_LINE_RESCUE_OUTER_SPEED        4500
#define CAR_LINE_RESCUE_INNER_SPEED        1400

/* 普通驾照障碍物掉头与返程终点检测参数。 */
#define CAR_OBSTACLE_DETECT_DISTANCE_CM     20.0f
#define CAR_OBSTACLE_CLEAR_DISTANCE_CM      30.0f
#define CAR_OBSTACLE_CONFIRM_SAMPLES         3U
#define CAR_AVOID_TURN_TARGET_DEG           180.0f

#endif
