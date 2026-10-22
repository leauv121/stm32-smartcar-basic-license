#ifndef FINAL_WORK_APP_CONFIG_H
#define FINAL_WORK_APP_CONFIG_H

/*
 * 功能开关只决定模块是否参与编译。
 * 传感器能否在任务中使用，还必须通过上电自检和运行时健康检查。
 */
#define APP_FEATURE_ERROR_MANAGER       1
#define APP_FEATURE_DIAGNOSTIC_DISPLAY  1
#define APP_FEATURE_SPEED_MEASUREMENT   1
#define APP_FEATURE_COLOR_SENSOR        1
#define APP_FEATURE_ULTRASONIC           1
#define APP_FEATURE_BATTERY_MONITOR      1
#define APP_FEATURE_NORMAL_MISSION       1
#define APP_FEATURE_ADVANCED_MISSION     1

#endif
