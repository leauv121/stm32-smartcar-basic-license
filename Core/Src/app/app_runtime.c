#include "app/app_runtime.h"

#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "app/app_controller.h"
#include "app/route_detector.h"
#include "config/app_config.h"
#include "config/car_parameters.h"
#include "control/line_follow_control.h"
#include "control/motion_control.h"
#include "control/turn_control.h"
#include "dma.h"
#include "hc_sr04.h"
#include "key.h"
#include "oled_driver.h"
#include "services/diagnostic_view.h"
#include "services/display_service.h"
#include "services/driver_algorithms.h"
#include "services/error_manager.h"
#include "services/hardware_adapter.h"
#include "services/indicator_service.h"
#include "services/safety_service.h"
#include "services/scheduler.h"
#include "services/self_test.h"
#include "services/sensor_service.h"
#include "services/uart_command_service.h"
#include "speed_measure.h"
#include "tb6612.h"
#include "tcs3472.h"
#include "tim.h"
#include "usart.h"
#include "mpu6050_dmp.h"

#define ADC_CHANNEL_COUNT 2U
#define HAZARD_HOLD_MS 1000U

static volatile uint16_t adc_data[ADC_CHANNEL_COUNT];
static volatile uint8_t mpu_ready_flag;
static volatile uint8_t adc_error_pending;
static DriverStatus adc_status;
static HardwareHealth hardware_health;
static IndicatorPattern last_indicator;
static IndicatorOutput indicator_output;
static ColorConfirmState color_confirm;
static uint32_t obstacle_hold_started_ms;
static uint8_t avoid_turn_started;
static uint8_t line_reacquired_pending;
static uint8_t self_test_announced;
static float pitch;
static float roll;
static float yaw;
static int16_t line_target_left;
static int16_t line_target_right;
static char line_mode;

static char LineFollowModeText(LineFollowMode mode)
{
    switch (mode)
    {
        case LINE_FOLLOW_RESCUE_LEFT: return 'L';
        case LINE_FOLLOW_RESCUE_RIGHT: return 'R';
        case LINE_FOLLOW_PID: return 'P';
        default: return 'F';
    }
}

static SelfTestInputs BuildSelfTestInputs(void)
{
    SelfTestInputs input;
    (void)memset(&input, 0, sizeof(input));
    input.adc = adc_status;
    input.mpu = hardware_health.mpu;
    input.color = hardware_health.color;
    input.ultrasonic = hardware_health.ultrasonic;
    input.speed = hardware_health.speed;
    input.battery = hardware_health.battery;
    input.display = hardware_health.display;
    input.color_enabled = APP_FEATURE_COLOR_SENSOR;
    input.ultrasonic_enabled = APP_FEATURE_ULTRASONIC;
    input.speed_enabled = APP_FEATURE_SPEED_MEASUREMENT;
    /* 当前板级 ADC 只配置了两路循迹，电池检测在增加分压通道前保持跳过。 */
    input.battery_enabled = 0U;
    return input;
}

static SensorColor ReadColor(uint32_t now_ms)
{
    COLOR_RGBC rgb = {0};
    COLOR_HSL hsl = {0};
    DriverColor raw = DRIVER_COLOR_NONE;
    DriverColor confirmed;

    if (TCS34725_GetRawData(&rgb) == 0U)
    {
        return SENSOR_COLOR_UNKNOWN;
    }
    RGB_normalize(&rgb);
    RGBtoHSL(&rgb, &hsl);
    if ((hsl.h <= 30U) || (hsl.h >= 330U)) raw = DRIVER_COLOR_RED;
    else if ((hsl.h >= 80U) && (hsl.h <= 160U)) raw = DRIVER_COLOR_GREEN;
    confirmed = DriverAlgorithms_ConfirmColor(&color_confirm, raw, 3U);
    if (confirmed == DRIVER_COLOR_RED)
    {
        SensorService_UpdateColor(SENSOR_COLOR_RED, now_ms);
    }
    else if (confirmed == DRIVER_COLOR_GREEN)
    {
        SensorService_UpdateColor(SENSOR_COLOR_GREEN, now_ms);
    }
    else if (raw == DRIVER_COLOR_NONE)
    {
        SensorService_UpdateColor(SENSOR_COLOR_OTHER, now_ms);
    }
    return SensorService_GetSnapshot()->color.value;
}

static RouteEvent DetectRouteEvent(void)
{
    const SensorSnapshot *sensor = SensorService_GetSnapshot();
    RouteDetectorInput input = {ROUTE_EVENT_NONE, 0U};
    if ((sensor->line.outer_left != 0U) && (sensor->line.outer_right != 0U))
    {
        input.candidate = ROUTE_POINT_A;
        input.candidate_valid = 1U;
    }
    else if (sensor->line.outer_left != 0U)
    {
        input.candidate = ROUTE_POINT_B;
        input.candidate_valid = 1U;
    }
    else if (sensor->line.outer_right != 0U)
    {
        input.candidate = ROUTE_POINT_C;
        input.candidate_valid = 1U;
    }
    else if (sensor->color.value == SENSOR_COLOR_RED)
    {
        input.candidate = ROUTE_POINT_D;
        input.candidate_valid = 1U;
    }
    return RouteDetector_Update(&input);
}

static void UpdateDisplay(const AppOutput *app_output, uint32_t now_ms)
{
    const ErrorRecord *record;
    const SensorSnapshot *sensor = SensorService_GetSnapshot();
    uint8_t index;

    if (ErrorManager_GetActiveCount() != 0U)
    {
        DiagnosticLines diagnostic;
        record = ErrorManager_GetDisplayRecord(now_ms);
        DiagnosticView_Format(record, 0U, ErrorManager_GetActiveCount(), &diagnostic);
        OLED_ShowString(0U, 0U, diagnostic.line[0], 16U);
        OLED_ShowString(0U, 2U, diagnostic.line[1], 16U);
        OLED_ShowString(0U, 4U, diagnostic.line[2], 16U);
        OLED_ShowString(0U, 6U, diagnostic.line[3], 16U);
    }
    else
    {
        DisplayModel model;
        DisplayLines lines;
        (void)memset(&model, 0, sizeof(model));
        model.state = app_output->state;
        model.mode = app_output->mode;
        model.left_speed = sensor->speed.left;
        model.right_speed = sensor->speed.right;
        model.outer_right = sensor->line.outer_right;
        model.outer_left = sensor->line.outer_left;
        model.line_target_left = line_target_left;
        model.line_target_right = line_target_right;
        model.line_mode = line_mode;
        model.distance_cm = sensor->distance_cm.value;
        model.battery_v = sensor->battery_v.value;
        model.yaw = sensor->yaw.value;
        model.color = sensor->color.value;
        DisplayService_FormatNormal(&model, &lines);
        for (index = 0U; index < 4U; index++)
        {
            OLED_ShowString(0U, (uint8_t)(index * 2U), lines.line[index], 16U);
        }
    }
}

static void ApplyIndicators(IndicatorOutput output)
{
    HAL_GPIO_WritePin(LeftLight_GPIO_Port, LeftLight_Pin,
                      output.left_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RightLight_GPIO_Port, RightLight_Pin,
                      output.right_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,
                      output.core_led_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void AppRuntime_Init(void)
{
    ErrorManager_Init();
    SafetyService_Init();
    IndicatorService_Init();
    SensorService_Init();
    RouteDetector_Init();
    LineFollowControl_Reset();
    line_reacquired_pending = 0U;
    line_target_left = 0;
    line_target_right = 0;
    line_mode = 'P';
    TB6612_Init();
    MotionControl_Apply(SafetyService_Resolve(MotionRequest_Stop(), 0U, 0U));
    AppController_Init();
    Scheduler_Init(HAL_GetTick());
    UartCommandService_Init(&huart3);

    adc_status = DRIVER_ERROR;
    if ((HAL_ADCEx_Calibration_Start(&hadc1) == HAL_OK) &&
        (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_data, ADC_CHANNEL_COUNT) == HAL_OK))
    {
        __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT | DMA_IT_TC);
        adc_status = DRIVER_OK;
    }
    else
    {
        ErrorManager_Report(ERR_MODULE_ADC, ERROR_CODE_INIT_FAILED,
                            ERROR_LEVEL_FATAL, HAL_GetTick(), 0U);
    }

    HC_SR04Init(&htim1, TIM_CHANNEL_4);
    HardwareAdapter_Init();
    HardwareAdapter_SetRuntimeStatus(DRIVER_NOT_READY, DRIVER_OK, DRIVER_NOT_READY);
    hardware_health = HardwareAdapter_GetHealth();
    SelfTest_Start(HAL_GetTick());
    OLED_Clear();
    UartCommandService_SendText("BOOT SELF TEST\r\n");
}

void AppRuntime_RunOnce(void)
{
    uint32_t now_ms = HAL_GetTick();
    SchedulerEvents events = Scheduler_Update(now_ms);
    AppInput app_input;
    AppOutput app_output;
    AppCommand command = {APP_COMMAND_NONE, APP_MODE_NONE};
    const SensorSnapshot *sensor;
    MotionRequest requested;
    AppState previous_state = AppController_GetState();
    NormalMissionState mission_state_before = NormalMission_GetState();
    uint8_t yaw_required;
    uint8_t line_sample_updated = 0U;
    uint8_t distance_sample_updated = 0U;

    (void)memset(&app_input, 0, sizeof(app_input));
    if (adc_error_pending != 0U)
    {
        adc_error_pending = 0U;
        adc_status = DRIVER_ERROR;
        ErrorManager_Report(ERR_MODULE_ADC, ERROR_CODE_COMMUNICATION,
                            ERROR_LEVEL_FATAL, now_ms, 0U);
    }
    if ((mpu_ready_flag != 0U) || (events.tick_10ms != 0U))
    {
        mpu_ready_flag = 0U;
        if ((hardware_health.mpu == DRIVER_OK) &&
            (MPU6050_DMP_GetData(&pitch, &roll, &yaw) == 0U))
        {
            SensorService_UpdateYaw(yaw, now_ms);
        }
    }

    if (events.tick_20ms != 0U)
    {
        uint32_t adc_state = HAL_ADC_GetState(&hadc1);
        if ((adc_status == DRIVER_OK) &&
            (HAL_DMA_GetState(&hdma_adc1) == HAL_DMA_STATE_BUSY) &&
            ((adc_state & (HAL_ADC_STATE_ERROR_INTERNAL |
                           HAL_ADC_STATE_ERROR_CONFIG |
                           HAL_ADC_STATE_ERROR_DMA)) == 0U))
        {
            SensorService_UpdateLine(adc_data[0], adc_data[1],
                DriverAlgorithms_LineDigitalIsBlack((uint8_t)HAL_GPIO_ReadPin(
                    LINE1_GPIO_Port, LINE1_Pin)),
                DriverAlgorithms_LineDigitalIsBlack((uint8_t)HAL_GPIO_ReadPin(
                    LINE4_GPIO_Port, LINE4_Pin)),
                now_ms);
            line_sample_updated = 1U;
        }
    }
    if (events.tick_100ms != 0U)
    {
        (void)ReadColor(now_ms);
        if (HC_SR04IsDataReady() != 0U)
        {
            SensorService_UpdateDistance(HC_SR04Read(), now_ms);
            distance_sample_updated = 1U;
            HardwareAdapter_SetRuntimeStatus(DRIVER_OK,
                                             hardware_health.speed,
                                             hardware_health.battery);
            hardware_health = HardwareAdapter_GetHealth();
        }
        HC_SR04Start();
        if ((Speed_Measure_GetLeftDataState() != 0U) ||
            (Speed_Measure_GetRightDataState() != 0U))
        {
            SensorService_UpdateWheelSpeeds(Speed_Measure_GetLeftData(),
                                            Speed_Measure_GetRightData(), now_ms);
            Speed_Measure_LeftDataProcessComplt();
            Speed_Measure_RightDataProcessComplt();
        }
    }

    if (SelfTest_IsComplete() == 0U)
    {
        SelfTestInputs self_test = BuildSelfTestInputs();
        (void)SelfTest_Update(&self_test, now_ms);
    }
    else if (self_test_announced == 0U)
    {
        self_test_announced = 1U;
        if (SelfTest_IsSafeToRun() == 0U)
        {
            UartCommandService_SendText("SELF TEST FAILED: CHECK OLED\r\n");
        }
        else if (ErrorManager_GetActiveCount() != 0U)
        {
            UartCommandService_SendText("SELF TEST DEGRADED: CHECK OLED\r\n");
        }
        else
        {
            UartCommandService_SendText("SELF TEST OK: SET MODE THEN START\r\n");
        }
    }

    yaw_required = (uint8_t)((AppController_GetState() == APP_STATE_RUNNING) &&
        ((AppController_GetMode() != APP_MODE_NORMAL) ||
         (NormalMission_GetState() == NORMAL_AVOID_TURN)));
    SensorService_CheckTimeouts(now_ms, yaw_required);
    app_input.self_test_complete = SelfTest_IsComplete();
    app_input.self_test_safe = SelfTest_IsSafeToRun();
    app_input.stop_latched = SafetyService_IsStopLatched();
    app_input.user_button_pressed = (uint8_t)Key_ProcAt(now_ms);
    if (UartCommandService_Poll(&command) != 0U)
    {
        app_input.command = command;
    }

    sensor = SensorService_GetSnapshot();
    if ((NormalMission_GetState() != NORMAL_OBSTACLE_HOLD) &&
        (NormalMission_GetState() != NORMAL_AVOID_TURN))
    {
        obstacle_hold_started_ms = 0U;
    }
    if (NormalMission_GetState() != NORMAL_AVOID_TURN)
    {
        avoid_turn_started = 0U;
    }
    app_input.normal.start_flash_done = indicator_output.completed;
    app_input.normal.red_detected = (uint8_t)(sensor->color.value == SENSOR_COLOR_RED);
    app_input.normal.green_detected = (uint8_t)(sensor->color.value == SENSOR_COLOR_GREEN);
    app_input.normal.obstacle_detected = (uint8_t)(sensor->distance_cm.valid &&
        (sensor->distance_cm.value <= CAR_OBSTACLE_DETECT_DISTANCE_CM));
    app_input.normal.obstacle_cleared = (uint8_t)(sensor->distance_cm.valid &&
        (sensor->distance_cm.value > CAR_OBSTACLE_CLEAR_DISTANCE_CM));
    app_input.normal.obstacle_sample_ready = distance_sample_updated;
    app_input.normal.line_reacquired = line_reacquired_pending;
    if ((NormalMission_GetState() == NORMAL_OBSTACLE_HOLD) &&
        (obstacle_hold_started_ms == 0U))
    {
        obstacle_hold_started_ms = now_ms;
    }
    app_input.normal.hazard_hold_done = (uint8_t)((obstacle_hold_started_ms != 0U) &&
        ((uint32_t)(now_ms - obstacle_hold_started_ms) >= HAZARD_HOLD_MS));
    if ((NormalMission_GetState() == NORMAL_AVOID_TURN) && (avoid_turn_started == 0U) &&
        sensor->yaw.valid)
    {
        TurnControl_Start(sensor->yaw.value, CAR_AVOID_TURN_TARGET_DEG,
                          TURN_RIGHT, now_ms);
        avoid_turn_started = 1U;
    }
    if (avoid_turn_started != 0U)
    {
        TurnResult turn = TurnControl_Update(sensor->yaw.value, sensor->yaw.valid, now_ms);
        app_input.normal.turn_complete = (uint8_t)(turn == TURN_COMPLETE);
        if ((turn == TURN_TIMEOUT) || (turn == TURN_SENSOR_ERROR))
        {
            ErrorManager_Report(ERR_MODULE_MISSION,
                                (turn == TURN_TIMEOUT) ? ERROR_CODE_DATA_TIMEOUT :
                                ERROR_CODE_VALUE_INVALID,
                                ERROR_LEVEL_FATAL, now_ms, 0U);
        }
    }
    app_input.normal.fatal_error = ErrorManager_HasFatal();
    app_input.advanced.route_event = DetectRouteEvent();
    app_input.advanced.fatal_error = ErrorManager_HasFatal();

    app_output = AppController_Update(&app_input, now_ms);
    if ((previous_state != APP_STATE_RUNNING) &&
        (app_output.state == APP_STATE_RUNNING))
    {
        RouteDetector_Reset();
        LineFollowControl_Reset();
        obstacle_hold_started_ms = 0U;
        avoid_turn_started = 0U;
        line_reacquired_pending = 0U;
    }
    requested = app_output.motion;
    if ((NormalMission_GetState() == NORMAL_REACQUIRE_LINE) &&
        (mission_state_before != NORMAL_REACQUIRE_LINE))
    {
        LineFollowControl_StartRecovery(LINE_FOLLOW_RESCUE_RIGHT);
        line_reacquired_pending = 0U;
    }
    if ((requested.mode == MOTION_DRIVE) && sensor->line.valid &&
        (NormalMission_GetState() == NORMAL_REACQUIRE_LINE))
    {
        LineFollowInput line_input = {sensor->line.left_adc, sensor->line.right_adc,
                                      sensor->line.outer_right, sensor->line.outer_left,
                                      line_sample_updated};
        LineFollowOutput line_output = LineFollowControl_Update(line_input);
        requested.left_target = line_output.left_target;
        requested.right_target = line_output.right_target;
        line_target_left = line_output.left_target;
        line_target_right = line_output.right_target;
        line_mode = LineFollowModeText(line_output.mode);
        line_reacquired_pending = (uint8_t)(line_output.mode == LINE_FOLLOW_PID);
        if (line_output.mode == LINE_FOLLOW_FAULT)
        {
            ErrorManager_Report(ERR_MODULE_LINE_SENSOR,
                                ERROR_CODE_VALUE_INVALID,
                                ERROR_LEVEL_FATAL, now_ms, 0U);
        }
    }
    else if ((requested.mode == MOTION_DRIVE) && sensor->line.valid &&
             (NormalMission_GetState() != NORMAL_AVOID_TURN))
    {
        LineFollowInput line_input = {sensor->line.left_adc, sensor->line.right_adc,
                                      sensor->line.outer_right, sensor->line.outer_left,
                                      line_sample_updated};
        LineFollowOutput line_output = LineFollowControl_Update(line_input);
        requested.left_target = line_output.left_target;
        requested.right_target = line_output.right_target;
        line_target_left = line_output.left_target;
        line_target_right = line_output.right_target;
        line_mode = LineFollowModeText(line_output.mode);
        line_reacquired_pending = 0U;
        if (line_output.mode == LINE_FOLLOW_FAULT)
        {
            ErrorManager_Report(ERR_MODULE_LINE_SENSOR,
                                ERROR_CODE_VALUE_INVALID,
                                ERROR_LEVEL_FATAL, now_ms, 0U);
        }
    }
    else if (NormalMission_GetState() != NORMAL_REACQUIRE_LINE)
    {
        line_reacquired_pending = 0U;
    }
    requested = SafetyService_Resolve(requested,
                                      app_output.user_stop_requested,
                                      app_output.mission_complete);
    MotionControl_Apply(requested);

    if (app_output.indicator != last_indicator)
    {
        IndicatorService_Start(app_output.indicator, now_ms);
        last_indicator = app_output.indicator;
    }
    indicator_output = IndicatorService_Update(now_ms);
    ApplyIndicators(indicator_output);

    if (events.tick_200ms != 0U)
    {
        UpdateDisplay(&app_output, now_ms);
    }
    if ((app_input.command.type != APP_COMMAND_NONE) || app_output.status_requested ||
        app_output.errors_requested)
    {
        if (app_input.command.type == APP_COMMAND_ACK)
        {
            SafetyService_Acknowledge();
        }
        UartCommandService_SendResult(&app_output);
    }
}

void AppRuntime_OnMpuReady(void)
{
    mpu_ready_flag = 1U;
}

void AppRuntime_OnAdcError(void)
{
    /* 中断只置位，故障登记与安全停车在主循环完成。 */
    adc_error_pending = 1U;
}

void AppRuntime_OnTimerPeriod(TIM_HandleTypeDef *timer)
{
    if (timer->Instance == TIM1) Hcsr04TimOverflowIsr();
    else if (timer->Instance == TIM4) Speed_MeasureTimOverflowHandler();
}

void AppRuntime_OnTimerCapture(TIM_HandleTypeDef *timer)
{
    if ((timer->Instance == TIM1) && (timer->Channel == HAL_TIM_ACTIVE_CHANNEL_4))
    {
        Hcsr04TimIcIsr();
    }
    else if (timer->Instance == TIM4)
    {
        if (timer->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
            Speed_MeasureTimIcHandler(TIM_CHANNEL_3);
        else if (timer->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
            Speed_MeasureTimIcHandler(TIM_CHANNEL_4);
    }
}

void AppRuntime_OnUartReceive(UART_HandleTypeDef *uart)
{
    UartCommandService_OnReceive(uart);
}

void AppRuntime_OnUartError(UART_HandleTypeDef *uart)
{
    UartCommandService_OnError(uart);
}
