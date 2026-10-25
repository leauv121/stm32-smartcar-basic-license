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
#define CAR_LINE_PID_BASE_SPEED           4300
#define CAR_LINE_PID_MAX_SPEED            6500
#define CAR_LINE_PID_INTEGRAL_LIMIT       8000
#define CAR_LINE_PID_ERROR_DEADBAND         60
#define CAR_LINE_PID_SOFT_START_STEP        60
#define CAR_LINE_CENTER_FAST_SPEED        5000
#define CAR_LINE_CAUTION_SPEED            3300
#define CAR_LINE_SPEED_DECEL_STEP          250
#define CAR_LINE_PID_OUTPUT_LIMIT         2600
#define CAR_LINE_CAUTION_OUTPUT_LIMIT     2400
#define CAR_LINE_CENTER_FAST_SAMPLES        10U
#define CAR_LINE_DEVIATION_SLOW_SAMPLES      3U
#define CAR_LINE_OUTER_CONFIRM_SAMPLES       2U
#define CAR_LINE_LOST_SEARCH_SAMPLES         5U

/* 外侧数字传感器触发急弯救线时，保持双轮前进以减少模式切换顿挫。 */
#define CAR_LINE_RESCUE_OUTER_SPEED        4600
#define CAR_LINE_RESCUE_INNER_SPEED         400
#define CAR_LINE_REACQUIRE_OUTER_SPEED     2400
#define CAR_LINE_REACQUIRE_INNER_SPEED      700
#define CAR_LINE_BLACK_ADC_MIN             2000U
#define CAR_LINE_RECENTER_ERROR_MAX         500
#define CAR_LINE_RECENTER_CONFIRM_SAMPLES     3U
#define CAR_LINE_RESCUE_CENTER_CONFIRM_SAMPLES 6U
#define CAR_LINE_CURVE_CONFIRM_SAMPLES       3U
#define CAR_LINE_CURVE_RELEASE_SAMPLES      3U
#define CAR_LINE_CURVE_ERROR_MIN            700
#define CAR_LINE_CURVE_PID_GAIN_NUM           3
#define CAR_LINE_CURVE_PID_GAIN_DEN           2
#define CAR_LINE_CURVE_SPEED              3800
#define CAR_LINE_CURVE_OUTPUT_LIMIT       2800
#define CAR_LINE_CURVE_MIN_WHEEL_SPEED    1000

/* 普通驾照障碍物掉头与返程终点检测参数。 */
#define CAR_OBSTACLE_DETECT_DISTANCE_CM     20.0f
#define CAR_OBSTACLE_CLEAR_DISTANCE_CM      30.0f
#define CAR_OBSTACLE_CONFIRM_SAMPLES         3U
#define CAR_AVOID_TURN_TARGET_DEG           190.0f
#define CAR_AVOID_TURN_LEFT_SPEED           3200
#define CAR_AVOID_TURN_RIGHT_SPEED         -3200

#endif
