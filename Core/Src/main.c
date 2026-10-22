/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : PID循迹、姿态计圈与自动掉头主程序
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tb6612.h"
#include "key.h"
#include "stdio.h"
#include "car_control.h"
#include "oled_driver.h"
#include "mpu6050.h"
#include "mpu6050_dmp.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  ADC_ERROR_NONE = 0,
  ADC_ERROR_CALIBRATION,
  ADC_ERROR_START,
  ADC_ERROR_DMA,
  ADC_ERROR_RUNTIME
} AdcErrorState;

typedef enum
{
  MPU_ERROR_NONE = 0,
  MPU_ERROR_NOT_FOUND,
  MPU_ERROR_DMP_INIT
} MpuErrorState;

typedef enum
{
  TRACK_WAIT_START = 0,
  TRACK_CW,
  TRACK_TURN_AROUND,
  TRACK_CCW,
  TRACK_COMPLETE,
  TRACK_ERROR
} TrackingMissionState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_CHANNEL_COUNT              2U
#define MPU_POLL_INTERVAL_MS          10U
#define TRACK_DISPLAY_INTERVAL_MS    200U
#define TRACK_UART_INTERVAL_MS       200U
#define TRACK_CW_TARGET_DEG        720.0f
#define TRACK_TURN_TARGET_DEG      180.0f
#define TRACK_CCW_TARGET_DEG       720.0f
#define TRACK_YAW_DIRECTION         (-1.0f)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile uint8_t control_tick_50ms = 0U;
volatile uint8_t mems_data_rdy_flag = 0U;
volatile uint8_t init_flag = 0U;
/* key.c 通过 extern 使用该值完成按键边沿检测。 */
uint8_t key_rval = 0U;
volatile uint16_t adc_dma_data[ADC_CHANNEL_COUNT] = {0U, 0U};
volatile AdcErrorState adc_error_state = ADC_ERROR_NONE;

static MpuErrorState mpu_error_state = MPU_ERROR_NONE;
static TrackingMissionState tracking_state = TRACK_WAIT_START;
static uint8_t adc_ready = 0U;
static uint8_t mpu_ready = 0U;
static uint8_t mpu_sample_valid = 0U;
static uint8_t tracking_yaw_valid = 0U;
static uint8_t mpu_detected_address = 0U;
static uint8_t mpu_init_result = 0xFFU;
static float Pitch = 0.0f;
static float Roll = 0.0f;
static float Yaw = 0.0f;
static float tracking_last_yaw = 0.0f;
static float tracking_progress = 0.0f;
static uint32_t last_mpu_poll_ms = 0U;
static uint32_t last_display_ms = 0U;
static uint32_t last_uart_ms = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static uint8_t Tracking_Mission_Is_Running(void);
static void Tracking_Mission_Reset(void);
static void Tracking_Mission_Start(void);
static void Tracking_Mission_Update_Yaw(float current_yaw);
static void Tracking_Mission_Process(uint8_t control_tick_due);
static const char *Tracking_State_Text(void);
static const char *Tracking_Error_Text(void);
static void OLED_Update_Tracking_Mission(void);
static void UART_Report_Tracking_Mission(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint8_t Tracking_Mission_Is_Running(void)
{
  return ((tracking_state == TRACK_CW) ||
          (tracking_state == TRACK_TURN_AROUND) ||
          (tracking_state == TRACK_CCW)) ? 1U : 0U;
}

/**
  * @brief  停止当前任务并回到等待状态
  * @note   USER按键停止时不会关闭ADC、DMA和MPU，便于继续观察数据。
  */
static void Tracking_Mission_Reset(void)
{
  tracking_state = TRACK_WAIT_START;
  tracking_progress = 0.0f;
  tracking_last_yaw = Yaw;
  tracking_yaw_valid = mpu_sample_valid;
  Line_Following_PID_Reset();
  Smart_Car_Stop();
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

/**
  * @brief  从顺时针阶段开始完整实验任务
  */
static void Tracking_Mission_Start(void)
{
  if ((adc_error_state != ADC_ERROR_NONE) ||
      (mpu_error_state != MPU_ERROR_NONE))
  {
    tracking_state = TRACK_ERROR;
    Smart_Car_Stop();
    return;
  }

  /* 外设尚在等待第一帧数据时保持等待，不把“未就绪”误报为硬件故障。 */
  if ((adc_ready == 0U) ||
      (mpu_ready == 0U) ||
      (mpu_sample_valid == 0U))
  {
    tracking_state = TRACK_WAIT_START;
    Smart_Car_Stop();
    return;
  }

  tracking_progress = 0.0f;
  tracking_last_yaw = Yaw;
  tracking_yaw_valid = 1U;
  Line_Following_PID_Reset();
  tracking_state = TRACK_CW;
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
  printf("TRACK MISSION START: CW 2 LAPS\r\n");
}

/**
  * @brief  使用连续Yaw增量推进计圈和掉头状态
  * @param  current_yaw DMP最新Yaw，范围约为-180到+180度
  */
static void Tracking_Mission_Update_Yaw(float current_yaw)
{
  float yaw_delta;
  float stage_delta;

  if (tracking_yaw_valid == 0U)
  {
    tracking_last_yaw = current_yaw;
    tracking_yaw_valid = 1U;
    return;
  }

  yaw_delta = current_yaw - tracking_last_yaw;
  if (yaw_delta > 180.0f)
  {
    yaw_delta -= 360.0f;
  }
  else if (yaw_delta < -180.0f)
  {
    yaw_delta += 360.0f;
  }
  tracking_last_yaw = current_yaw;

  if ((tracking_state == TRACK_WAIT_START) ||
      (tracking_state == TRACK_COMPLETE) ||
      (tracking_state == TRACK_ERROR))
  {
    return;
  }

  stage_delta = yaw_delta * TRACK_YAW_DIRECTION;
  if (tracking_state == TRACK_CCW)
  {
    stage_delta = -stage_delta;
  }

  tracking_progress += stage_delta;
  if (tracking_progress < 0.0f)
  {
    tracking_progress = 0.0f;
  }

  if ((tracking_state == TRACK_CW) &&
      (tracking_progress >= TRACK_CW_TARGET_DEG))
  {
    Smart_Car_Stop();
    Line_Following_PID_Reset();
    tracking_state = TRACK_TURN_AROUND;
    tracking_progress = 0.0f;
    tracking_last_yaw = current_yaw;
  }
  else if ((tracking_state == TRACK_TURN_AROUND) &&
           (tracking_progress >= TRACK_TURN_TARGET_DEG))
  {
    Smart_Car_Stop();
    Line_Following_PID_Reset();
    tracking_state = TRACK_CCW;
    tracking_progress = 0.0f;
    tracking_last_yaw = current_yaw;
  }
  else if ((tracking_state == TRACK_CCW) &&
           (tracking_progress >= TRACK_CCW_TARGET_DEG))
  {
    Smart_Car_Stop();
    Line_Following_PID_Reset();
    tracking_state = TRACK_COMPLETE;
    tracking_progress = TRACK_CCW_TARGET_DEG;
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
  }
}

/**
  * @brief  单一电机控制出口：PID循迹、右转掉头或停车
  * @param  control_tick_due 50ms控制节拍到达标志
  */
static void Tracking_Mission_Process(uint8_t control_tick_due)
{
  if ((adc_error_state != ADC_ERROR_NONE) ||
      (mpu_error_state != MPU_ERROR_NONE))
  {
    tracking_state = TRACK_ERROR;
  }

  switch (tracking_state)
  {
    case TRACK_CW:
    case TRACK_CCW:
      if (control_tick_due == 1U)
      {
        uint16_t left_adc = adc_dma_data[0];
        uint16_t right_adc = adc_dma_data[1];
        Line_Following_PID(left_adc, right_adc);
      }
      break;

    case TRACK_TURN_AROUND:
      Smart_Car_Turn_Right();
      break;

    case TRACK_WAIT_START:
    case TRACK_COMPLETE:
    case TRACK_ERROR:
    default:
      Smart_Car_Stop();
      break;
  }
}

static const char *Tracking_State_Text(void)
{
  switch (tracking_state)
  {
    case TRACK_CW:
      return "CW";
    case TRACK_TURN_AROUND:
      return "TURN";
    case TRACK_CCW:
      return "CCW";
    case TRACK_COMPLETE:
      return "DONE";
    case TRACK_ERROR:
      return "ERR";
    case TRACK_WAIT_START:
    default:
      return "WAIT";
  }
}

static const char *Tracking_Error_Text(void)
{
  switch (adc_error_state)
  {
    case ADC_ERROR_CALIBRATION:
      return "ADC CAL ERR";
    case ADC_ERROR_START:
      return "ADC START ERR";
    case ADC_ERROR_DMA:
      return "ADC DMA ERR";
    case ADC_ERROR_RUNTIME:
      return "ADC RUN ERR";
    case ADC_ERROR_NONE:
    default:
      break;
  }

  if (mpu_error_state == MPU_ERROR_NOT_FOUND)
  {
    return "MPU NOT FOUND";
  }
  if (mpu_error_state == MPU_ERROR_DMP_INIT)
  {
    return "DMP INIT FAIL";
  }
  return "NO ERROR";
}

/**
  * @brief  低频刷新OLED；固定宽度覆盖旧内容，不在高优先级路径清屏
  */
static void OLED_Update_Tracking_Mission(void)
{
  char line[17];
  int32_t yaw_x10;
  uint32_t yaw_magnitude;
  uint32_t target_deg;
  uint32_t progress_deg;

  if (tracking_state == TRACK_ERROR)
  {
    OLED_ShowString(0, 0, "TRACK ERROR     ", 16);
    (void)snprintf(line, sizeof(line), "%-16s", Tracking_Error_Text());
    OLED_ShowString(0, 2, line, 16);
    OLED_ShowString(0, 4, "MOTOR STOP      ", 16);
    OLED_ShowString(0, 6, "CHECK WIRING    ", 16);
    return;
  }

  (void)snprintf(line, sizeof(line), "MODE:%-11s", Tracking_State_Text());
  OLED_ShowString(0, 0, line, 16);
  (void)snprintf(line, sizeof(line), "L:%4u R:%4u   ",
                 adc_dma_data[0], adc_dma_data[1]);
  OLED_ShowString(0, 2, line, 16);

  target_deg = (tracking_state == TRACK_TURN_AROUND) ?
               (uint32_t)TRACK_TURN_TARGET_DEG :
               (uint32_t)TRACK_CW_TARGET_DEG;
  progress_deg = (tracking_progress > (float)target_deg) ?
                 target_deg : (uint32_t)tracking_progress;
  (void)snprintf(line, sizeof(line), "DEG:%3lu/%3lu   ",
                 (unsigned long)progress_deg,
                 (unsigned long)target_deg);
  OLED_ShowString(0, 4, line, 16);

  yaw_x10 = (Yaw >= 0.0f) ?
            (int32_t)(Yaw * 10.0f + 0.5f) :
            (int32_t)(Yaw * 10.0f - 0.5f);
  yaw_magnitude = (yaw_x10 < 0) ?
                  (uint32_t)(-yaw_x10) : (uint32_t)yaw_x10;
  (void)snprintf(line, sizeof(line), "YAW:%c%3lu.%1lu   ",
                 (yaw_x10 < 0) ? '-' : '+',
                 (unsigned long)(yaw_magnitude / 10U),
                 (unsigned long)(yaw_magnitude % 10U));
  OLED_ShowString(0, 6, line, 16);
}

/**
  * @brief  低频串口报告，避免打印速度超过MPU FIFO消费速度
  */
static void UART_Report_Tracking_Mission(void)
{
  printf("TRACK:%s L:%u R:%u PID:%ld YAW:%.2f DEG:%.1f\r\n",
         Tracking_State_Text(),
         adc_dma_data[0],
         adc_dma_data[1],
         (long)Line_Following_PID_GetOutput(),
         Yaw,
         tracking_progress);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint8_t key_value;
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();

  /* USER CODE BEGIN 2 */
  TB6612_Init();
  Smart_Car_Stop();
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

  OLED_Init();
  OLED_Clear();
  OLED_ShowString(0, 0, "TRACK INIT      ", 16);

  /*
   * 按指导书顺序启动ADC和DMA。
   * DMA循环搬运两个Rank：左侧PB0/IN8在下标0，右侧PA7/IN7在下标1。
   */
  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
  {
    adc_error_state = ADC_ERROR_CALIBRATION;
  }
  else if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    adc_error_state = ADC_ERROR_START;
  }
  else if (HAL_ADC_Start_DMA(&hadc1,
                             (uint32_t *)adc_dma_data,
                             ADC_CHANNEL_COUNT) != HAL_OK)
  {
    adc_error_state = ADC_ERROR_DMA;
  }
  else
  {
    adc_ready = 1U;
    printf("ADC DMA START OK: LEFT_IN8 RIGHT_IN7\r\n");
  }

  /*
   * 恢复MPU6050姿态链路：地址探测、DMP初始化、外部中断加10ms主动补读。
   * OLED和串口不放在FIFO消费路径中，防止再次出现生产快于消费的问题。
   */
  mpu_detected_address = MPU6050_DetectAddress();
  if (mpu_detected_address == 0U)
  {
    mpu_error_state = MPU_ERROR_NOT_FOUND;
    printf("MPU6050 not found at 0x68 or 0x69\r\n");
  }
  else
  {
    mpu_init_result = MPU6050_DMP_Init();
    if (mpu_init_result == 0U)
    {
      mpu_ready = 1U;
      init_flag = 1U;
      printf("MPU6050 DMP init success at 0x%02X\r\n",
             mpu_detected_address);
    }
    else
    {
      mpu_error_state = MPU_ERROR_DMP_INIT;
      printf("MPU6050 DMP init failed, addr:0x%02X code:%u\r\n",
             mpu_detected_address,
             mpu_init_result);
    }
  }

  if ((adc_error_state != ADC_ERROR_NONE) ||
      (mpu_error_state != MPU_ERROR_NONE))
  {
    tracking_state = TRACK_ERROR;
  }

  last_mpu_poll_ms = HAL_GetTick();
  last_display_ms = last_mpu_poll_ms;
  last_uart_ms = last_mpu_poll_ms;
  OLED_Update_Tracking_Mission();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint8_t control_tick_due = 0U;
    uint32_t now_ms = HAL_GetTick();

    /*
     * 最高优先级：及时消费MPU FIFO。
     * 驱动内部会有界追读并采用较新姿态；此处不打印、不刷新OLED。
     */
    if ((mpu_ready == 1U) &&
        ((mems_data_rdy_flag == 1U) ||
         ((now_ms - last_mpu_poll_ms) >= MPU_POLL_INTERVAL_MS)))
    {
      mems_data_rdy_flag = 0U;
      last_mpu_poll_ms = now_ms;
      if (MPU6050_DMP_GetData(&Pitch, &Roll, &Yaw) == 0U)
      {
        mpu_sample_valid = 1U;
        Tracking_Mission_Update_Yaw(Yaw);
      }
    }

    /* USER按键：等待/完成时启动，运行时立即停车，错误时不掩盖故障。 */
    key_value = Key_Proc();
    if (key_value == 1U)
    {
      if (Tracking_Mission_Is_Running() == 1U)
      {
        Tracking_Mission_Reset();
        printf("TRACK MISSION STOPPED BY USER\r\n");
      }
      else if ((tracking_state == TRACK_WAIT_START) ||
               (tracking_state == TRACK_COMPLETE))
      {
        Tracking_Mission_Start();
      }
    }

    /* 原子取走一次50ms请求；阻塞后不补跑多个PID周期。 */
    __disable_irq();
    if (control_tick_50ms == 1U)
    {
      control_tick_50ms = 0U;
      control_tick_due = 1U;
    }
    __enable_irq();

    Tracking_Mission_Process(control_tick_due);

    now_ms = HAL_GetTick();
    if ((now_ms - last_display_ms) >= TRACK_DISPLAY_INTERVAL_MS)
    {
      last_display_ms = now_ms;
      OLED_Update_Tracking_Mission();
    }

    if ((now_ms - last_uart_ms) >= TRACK_UART_INTERVAL_MS)
    {
      last_uart_ms = now_ms;
      UART_Report_Tracking_Mission();
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if ((GPIO_Pin == MEMS_INT_Pin) && (init_flag == 1U))
  {
    mems_data_rdy_flag = 1U;
  }
}

/* DMA/ADC错误回调只置状态；主循环负责停车和显示，避免中断内I2C阻塞。 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    adc_error_state = ADC_ERROR_RUNTIME;
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number.
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  (void)file;
  (void)line;
  /* USER CODE END 6 */
}
#endif
