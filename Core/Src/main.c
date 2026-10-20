/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief    : Main program body
	* @retval int  
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tb6612.h"
#include "key.h"
#include "stdio.h"
#include "string.h"
#include "car_control.h"
#include "speed_measure.h"
#include "hc_sr04.h"
#include "soft_i2c.h"
#include "oled_driver.h"
#include "tcs3472.h"
#include "mpu6050.h"
#include "mpu6050_dmp.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  CAR_FORWARD = 0,
  CAR_LEFT,
  CAR_RIGHT,
  CAR_BACKWARD,
  CAR_AVOID_RIGHT,
  CAR_STOP
} CarState;

typedef enum
{
  POSE_IDLE = 0,
  POSE_WAITING,
  POSE_ROTATING,
  POSE_PAUSED_BY_OBSTACLE,
  POSE_COMPLETE
} PoseTaskState;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PWM_PERIOD 7199U
#define OBSTACLE_DISTANCE_CM 35.0f
#define AVOID_TURN_TICKS 30U
#define COLOR_MARGIN 20U
#define COLOR_CLEAR_MIN 100U
#define COLOR_FORWARD_SPEED_PERCENT 50U
#define OBSTACLE_RESUME_DISTANCE_CM 40.0f
#define POSE_TURN_INTERVAL_MS 1000U
#define POSE_TURN_TARGET_DEG 90.0f
/* 先保证 FIFO 实时性，再做物理标定；正常情况下不应靠比例系数补偿漏帧。 */
#define POSE_YAW_SCALE 1.0f
#define POSE_YAW_DIRECTION -1.0f
#define POSE_TOTAL_TURNS 12U
#define MPU_POLL_INTERVAL_MS 10U
#define POSE_DISPLAY_INTERVAL_MS 200U
#define POSE_UART_REPORT_INTERVAL_MS 200U
#define COLOR_SAMPLE_INTERVAL_MS 60U
#define COLOR_UART_REPORT_INTERVAL_MS 300U

/* 超声波避障总开关：0=完全静默，1=恢复初始化、测距和避障控制。 */
#define ULTRASONIC_AVOIDANCE_ENABLE 0U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define USART3_RX_BUFFER_NUM 64U

volatile uint8_t control_tick_50ms = 0U;
uint8_t measure_flag = 0U;
uint8_t turn_count = 0U;
uint8_t auto_enabled = 0U;
/* 颜色识别始终运行，但复位后禁止绿色自行启动电机。 */
uint8_t color_control_armed = 0U;
float front_distance = 0.0f;


uint8_t usart3_rx_temp;
uint8_t usart3_rx_buffer[USART3_RX_BUFFER_NUM];
volatile uint8_t usart3_rx_cmplt_flag = 0U;
volatile uint16_t usart3_rx_count = 0U;

uint8_t car_speed_percent = 50U;
uint16_t car_speed_pwm = 3600U;
uint8_t key_rval = 0U;
volatile uint8_t tim3_flag = 0U;
uint8_t time_count = 0U;
uint8_t light_state = 1U;
CarState car_state = CAR_STOP;

COLOR_RGBC rgb; // RGB 
COLOR_HSL hsl; // HSL 

volatile uint8_t mems_data_rdy_flag = 0U;
volatile uint8_t init_flag = 0U;
volatile uint8_t start_flag = 0U;
uint8_t mpu_init_result = 0xFFU;
uint8_t mpu_detected_address = 0U;
float Pitch = 0.0f;
float Roll = 0.0f;
float Yaw = 0.0f;
PoseTaskState pose_task_state = POSE_IDLE;
uint8_t pose_turn_index = 0U;
uint8_t pose_yaw_valid = 0U;
uint8_t obstacle_present = 0U;
float pose_last_yaw = 0.0f;
float pose_turn_progress = 0.0f;
uint32_t pose_wait_started_ms = 0U;
uint32_t last_mpu_poll_ms = 0U;
uint32_t last_pose_display_ms = 0U;
uint32_t last_pose_uart_ms = 0U;
uint32_t last_color_sample_ms = 0U;
uint32_t last_color_uart_ms = 0U;
static const uint8_t uart_direct_ack_message[] = "UART DIRECT ACK\r\n";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Car_Control(void);
static void Light_Blink_Proc(void);
static uint8_t Parse_Speed_Command(const char *command, uint8_t *speed_percent);
static const char *Car_State_Text(void);
static void UART_Send_Status(void);
static uint8_t Pose_Task_Is_Active(void);
static void Pose_Task_Stop(void);
static void Pose_Task_Update_Yaw(float current_yaw);
static void Pose_Task_Process(void);
static void OLED_Show_Angle(uint8_t row, char *label, float angle);
static void OLED_Update_Pose(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim)
{
	/* TIM1_CH4 原本用于 HC-SR04 回波捕获；关闭避障后不再处理该中断。 */
#if ULTRASONIC_AVOIDANCE_ENABLE
	if ((htim == &htim1) &&
	    (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4))
	{
		Hcsr04TimIcIsr();
	}
	else
#endif
	if (htim == &htim4)
	{
		/* TIM4_CH3/CH4 保留给左右轮测速。 */
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
		{
			Speed_MeasureTimIcHandler(TIM_CHANNEL_3);
		}
		else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
		{
			Speed_MeasureTimIcHandler(TIM_CHANNEL_4);
		}
	}
}
/**
  * @brief  ????????????????USART3???????????????
  * @param  None
  * @retval None
  * @note   ??????????????????????????????????,???????????????????
  */
void USART3_Start_Receive(void)
{
    memset(usart3_rx_buffer, 0, sizeof(usart3_rx_buffer));
    usart3_rx_cmplt_flag = 0U;
    usart3_rx_count = 0U;

    HAL_UART_Receive_IT(&huart3, &usart3_rx_temp, 1U);
}

/**
  * @brief  
  * @param  
  * @retval 
  */
uint16_t USART3_Get_Rx_Num(void)
{
	return usart3_rx_count;
}

/**
  * @brief  ???USART3??????????????'???
  * @param  None
  * @retval USART3??????????????'???
  */
uint8_t * USART3_Get_RxBuffer_Pointer(void)
{
	USART3_Start_Receive();
	return usart3_rx_buffer;
}

/**
  * @brief  ???USART3??????
  * @param  None
  * @retval 0 ????????????????? 1 ??????????
  */
uint8_t USART3_Get_RxState(void)
{
	return usart3_rx_cmplt_flag;
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
#if ULTRASONIC_AVOIDANCE_ENABLE
  if (htim->Instance == TIM1)
  {
    Hcsr04TimOverflowIsr();
  }
  else
#endif
  if (htim->Instance == TIM3)
  {
    /* TIM3 只产生灯光节拍标志，耗时工作放到主循环。 */
    tim3_flag = 1;
  }
	else if (htim->Instance == TIM4)
	{
		Speed_MeasureTimOverflowHandler();
		
	}
	
}

static void Light_Blink_Proc(void)
{
  if (tim3_flag == 1)
  {
    tim3_flag = 0;
    time_count++;

    if ((car_state == CAR_LEFT ||
         car_state == CAR_RIGHT ||
         car_state == CAR_AVOID_RIGHT) &&
        time_count >= 5)
    {
      time_count = 0;
      light_state ^= 1;
    }
    else if (car_state == CAR_BACKWARD && time_count >= 3)
    {
      time_count = 0;
      light_state ^= 1;
    }
  }
}

static void Car_Control(void)
{
  GPIO_PinState blink_output;

  if (light_state == 1)
  {
    blink_output = GPIO_PIN_SET;
  }
  else
  {
    blink_output = GPIO_PIN_RESET;
  }

  if (car_state == CAR_FORWARD)
  {
    TB6612_SetSpeed(MOTOR_LEFT, 0, car_speed_pwm);
    TB6612_SetSpeed(MOTOR_RIGHT, 0, car_speed_pwm);
    HAL_GPIO_WritePin(LeftLight_GPIO_Port, LeftLight_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RightLight_GPIO_Port, RightLight_Pin, GPIO_PIN_RESET);
  }
  else if (car_state == CAR_LEFT)
  {
    TB6612_SetSpeed(MOTOR_LEFT, 0, 0);
    TB6612_SetSpeed(MOTOR_RIGHT, 0, car_speed_pwm);
    HAL_GPIO_WritePin(LeftLight_GPIO_Port, LeftLight_Pin, blink_output);
    HAL_GPIO_WritePin(RightLight_GPIO_Port, RightLight_Pin, GPIO_PIN_RESET);
  }
  else if ((car_state == CAR_RIGHT) || (car_state == CAR_AVOID_RIGHT))
  {
    TB6612_SetSpeed(MOTOR_LEFT, 0, car_speed_pwm);
    TB6612_SetSpeed(MOTOR_RIGHT, 0, 0);
    HAL_GPIO_WritePin(LeftLight_GPIO_Port, LeftLight_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RightLight_GPIO_Port, RightLight_Pin, blink_output);
  }
  else if (car_state == CAR_BACKWARD)
  {
    TB6612_SetSpeed(MOTOR_LEFT, 1, car_speed_pwm);
    TB6612_SetSpeed(MOTOR_RIGHT, 1, car_speed_pwm);
    HAL_GPIO_WritePin(LeftLight_GPIO_Port, LeftLight_Pin, blink_output);
    HAL_GPIO_WritePin(RightLight_GPIO_Port, RightLight_Pin, blink_output);
  }
  else if (car_state == CAR_STOP)
  {
    TB6612_SetSpeed(MOTOR_LEFT, 0, 0);
    TB6612_SetSpeed(MOTOR_RIGHT, 0, 0);
    HAL_GPIO_WritePin(LeftLight_GPIO_Port, LeftLight_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RightLight_GPIO_Port, RightLight_Pin, GPIO_PIN_SET);
  }
}

static uint8_t Parse_Speed_Command(const char *command, uint8_t *speed_percent)
{
  const char prefix[] = "SPEED=";
  uint16_t value = 0U;
  uint16_t index = sizeof(prefix) - 1U;

  if (strncmp(command, prefix, sizeof(prefix) - 1U) != 0)
  {
    return 0U;
  }

  if (command[index] == '\0')
  {
    return 0U;
  }

  while (command[index] != '\0')
  {
    if ((command[index] < '0') || (command[index] > '9'))
    {
      return 0U;
    }

    value = (uint16_t)(value * 10U + (uint16_t)(command[index] - '0'));
    if (value > 100U)
    {
      return 0U;
    }
    index++;
  }

  *speed_percent = (uint8_t)value;
  return 1U;
}

static const char *Car_State_Text(void)
{
  if (car_state == CAR_FORWARD)
  {
    return "GOING_FORWARD";
  }
  if (car_state == CAR_LEFT)
  {
    return "TURNING_LEFT";
  }
  if (car_state == CAR_RIGHT)
  {
    return "TURNING_RIGHT";
  }
  if (car_state == CAR_BACKWARD)
  {
    return "GOING_BACKWARD";
  }
  if (car_state == CAR_AVOID_RIGHT)
  {
    return "AVOIDING_RIGHT";
  }
  return "STOPPED";
}

static void UART_Send_Status(void)
{
  printf("STATE:%s SPEED:%u\r\n", Car_State_Text(), car_speed_percent);
}

static uint8_t Pose_Task_Is_Active(void)
{
  return ((pose_task_state == POSE_WAITING) ||
          (pose_task_state == POSE_ROTATING) ||
          (pose_task_state == POSE_PAUSED_BY_OBSTACLE)) ? 1U : 0U;
}

static void Pose_Task_Stop(void)
{
  pose_task_state = POSE_IDLE;
  pose_turn_progress = 0.0f;
  pose_yaw_valid = 0U;
  car_state = CAR_STOP;
}

static void Pose_Task_Update_Yaw(float current_yaw)
{
  float yaw_delta;
  float pose_target_progress;

  /* 空闲或全部完成后只同步基准，不再累计任务角度。 */
  if ((pose_task_state == POSE_IDLE) ||
      (pose_task_state == POSE_COMPLETE))
  {
    pose_last_yaw = current_yaw;
    pose_yaw_valid = 1U;
    return;
  }

  /* 第一次旋转前的 1 秒等待不属于转角；后续等待要统计停车惯性。 */
  if ((pose_task_state == POSE_WAITING) &&
      (pose_turn_index == 0U))
  {
    pose_last_yaw = current_yaw;
    pose_yaw_valid = 1U;
    return;
  }

  if (pose_yaw_valid == 0U)
  {
    pose_last_yaw = current_yaw;
    pose_yaw_valid = 1U;
    return;
  }

  yaw_delta = current_yaw - pose_last_yaw;
  if (yaw_delta > 180.0f)
  {
    yaw_delta -= 360.0f;
  }
  else if (yaw_delta < -180.0f)
  {
    yaw_delta += 360.0f;
  }
  pose_last_yaw = current_yaw;

  /* 保留旋转方向：顺时针增加进度，反向回弹会抵消已经累计的角度。 */
  pose_turn_progress += yaw_delta * POSE_YAW_DIRECTION * POSE_YAW_SCALE;
  if (pose_turn_progress < 0.0f)
  {
    pose_turn_progress = 0.0f;
  }

  /* 等待或避障暂停期间只吸收惯性误差，不在此时完成下一段。 */
  if (pose_task_state != POSE_ROTATING)
  {
    return;
  }

  /* 使用整项任务的绝对累计目标：90、180、270……1080 度。 */
  pose_target_progress = ((float)(pose_turn_index + 1U)) *
                         POSE_TURN_TARGET_DEG;
  if (pose_turn_progress >= pose_target_progress)
  {
    pose_turn_index++;
    car_state = CAR_STOP;

    if (pose_turn_index >= POSE_TOTAL_TURNS)
    {
      pose_task_state = POSE_COMPLETE;
      printf("POSE TASK COMPLETE\r\n");
    }
    else
    {
      pose_wait_started_ms = HAL_GetTick();
      pose_task_state = POSE_WAITING;
      printf("POSE TURN:%u/%u\r\n", pose_turn_index, POSE_TOTAL_TURNS);
    }
  }
}

static void Pose_Task_Process(void)
{
  if ((pose_task_state == POSE_WAITING) &&
      ((HAL_GetTick() - pose_wait_started_ms) >= POSE_TURN_INTERVAL_MS))
  {
    if (obstacle_present == 1U)
    {
      pose_task_state = POSE_PAUSED_BY_OBSTACLE;
      car_state = CAR_STOP;
    }
    else
    {
      pose_last_yaw = Yaw;
      pose_yaw_valid = 1U;
      pose_task_state = POSE_ROTATING;
      car_state = CAR_RIGHT;
    }
  }
  else if (pose_task_state == POSE_PAUSED_BY_OBSTACLE)
  {
    car_state = CAR_STOP;
    if (obstacle_present == 0U)
    {
      pose_last_yaw = Yaw;
      pose_yaw_valid = 1U;
      pose_task_state = POSE_ROTATING;
      car_state = CAR_RIGHT;
    }
  }
  else if (pose_task_state == POSE_ROTATING)
  {
    car_state = CAR_RIGHT;
  }
}

static void OLED_Show_Angle(uint8_t row, char *label, float angle)
{
  int32_t angle_x10;
  uint32_t magnitude;

  if (angle >= 0.0f)
  {
    angle_x10 = (int32_t)(angle * 10.0f + 0.5f);
  }
  else
  {
    angle_x10 = (int32_t)(angle * 10.0f - 0.5f);
  }

  if (angle_x10 > 9999)
  {
    angle_x10 = 9999;
  }
  else if (angle_x10 < -9999)
  {
    angle_x10 = -9999;
  }

  OLED_ShowString(0, row, label, 16);
  OLED_ShowChar(48, row, (angle_x10 < 0) ? '-' : '+', 16);
  magnitude = (angle_x10 < 0) ? (uint32_t)(-angle_x10) : (uint32_t)angle_x10;
  OLED_ShowNum(56, row, magnitude / 10U, 3);
  OLED_ShowChar(80, row, '.', 16);
  OLED_ShowNum(88, row, magnitude % 10U, 1);
  OLED_ShowString(96, row, "    ", 16);
}

static void OLED_Update_Pose(void)
{
  OLED_Show_Angle(0, "PITCH:", Pitch);
  OLED_Show_Angle(2, "ROLL :", Roll);
  OLED_Show_Angle(4, "YAW  :", Yaw);
  OLED_ShowString(0, 6, "TURN:", 16);
  OLED_ShowNum(40, 6, pose_turn_index, 2);
  OLED_ShowString(56, 6, "/12     ", 16);
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  uint8_t * usart3_rxbuffer_pointer; /* USART3 接收缓冲区首地址 */
  uint16_t adc_value = 0U;           /* ADC1 通道3的原始转换结果，范围为0~4095 */
  float voltage_value = 0.0f;        /* 根据分压比例还原出的实际电池电压 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  MX_TIM4_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
	/* ===== 1. 执行机构和基础周期任务初始化 ===== */
	TB6612_Init();	
	HAL_TIM_Base_Start_IT(&htim3);

	/* ===== 2. 串口接收和发送链路诊断 ===== */
	usart3_rxbuffer_pointer = USART3_Get_RxBuffer_Pointer();
	printf("READY\r\n");

	/* STM32F1 在首次采样前执行一次 ADC 自校准，以减小转换偏差。 */
	if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
	{
		printf("ADC CALIBRATION FAILED\r\n");
	}

	/* ===== 3. 轮速测量初始化 ===== */
	Speed_MeasureInit();

	/*
	 * ===== 4. 超声波避障初始化 =====
	 * 当前 ULTRASONIC_AVOIDANCE_ENABLE=0，模块保持静默：
	 * 不启动 TIM1 捕获、不发送 TRIG、不读取距离，也不干预 car_state。
	 */
#if ULTRASONIC_AVOIDANCE_ENABLE
	HC_SR04Init(&htim1, TIM_CHANNEL_4);
#else
	measure_flag = 0U;
	obstacle_present = 0U;
#endif
	
/* ===== 5. OLED 初始化后先显示零角度，证明显示链路正常 ===== */
OLED_Init();
OLED_Update_Pose();

/* ===== 6. 颜色传感器初始化；颜色仍参与红停、绿行控制 ===== */
if (TCS34725_Init()) 
{
	printf("TCS34725 init success\n");
} 
else 
{
	printf("TCS34725 init failed\n");
}

/* ===== 7. MPU6050 地址探测与 DMP 初始化 =====
 * 先探测 AD0=0 对应的 0x68，再探测 AD0=1 对应的 0x69。
 * 探测成功后，底层所有 MPU 读写都会自动使用找到的地址。
 */
mpu_detected_address = MPU6050_DetectAddress();
if (mpu_detected_address == 0U)
{
	init_flag = 0U;
	mpu_init_result = 10U;
	printf("MPU6050 not found at 0x68 or 0x69\r\n");

	OLED_Clear();
	OLED_ShowString(0, 0, "MPU NOT FOUND", 16);
	OLED_ShowString(0, 2, "SCAN: 68 69", 16);
	OLED_ShowString(0, 4, "PB6: SCL", 16);
	OLED_ShowString(0, 6, "PB7: SDA", 16);
}
else
{
	mpu_init_result = MPU6050_DMP_Init();
	if (mpu_init_result == 0U)
	{
		init_flag = 1U;
		printf("MPU6050 DMP init success at 0x%02X\r\n",
		       mpu_detected_address);
		OLED_Update_Pose();
	}
	else
	{
		init_flag = 0U;
		printf("MPU6050 DMP init failed, addr:0x%02X code:%u\r\n",
		       mpu_detected_address,
		       mpu_init_result);

		OLED_Clear();
		OLED_ShowString(0, 0, "MPU6050 ERROR", 16);
		OLED_ShowString(0, 2, "CODE:", 16);
		OLED_ShowNum(48, 2, mpu_init_result, 2);
		OLED_ShowString(0, 4,
		                (mpu_detected_address == 0x69U) ?
		                "FOUND: 0x69" : "FOUND: 0x68",
		                16);
		OLED_ShowString(0, 6, "DMP INIT FAIL", 16);
	}
}

/* 各周期任务从同一时间基准开始，避免初始化结束后集中触发。 */
last_mpu_poll_ms = HAL_GetTick();
last_pose_display_ms = last_mpu_poll_ms;
last_pose_uart_ms = last_mpu_poll_ms;
last_color_sample_ms = last_mpu_poll_ms;
last_color_uart_ms = last_mpu_poll_ms;


	
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {	
		/*
		 * ===== A. MPU6050 数据链 =====
		 * PA8 外部中断只置位 mems_data_rdy_flag；主循环负责 IIC 读取。
		 * 另外每 10ms 主动补读一次，防止中断边沿丢失或 INT 持续有效。
		 * 姿态控制每帧更新，OLED 和串口按较低频率刷新，避免堵塞 FIFO。
		 */
		if ((init_flag == 1U) &&
		    ((mems_data_rdy_flag == 1U) ||
		     ((HAL_GetTick() - last_mpu_poll_ms) >= MPU_POLL_INTERVAL_MS)))
		{
			mems_data_rdy_flag = 0U;
			last_mpu_poll_ms = HAL_GetTick();

			if (MPU6050_DMP_GetData(&Pitch, &Roll, &Yaw) == 0U)
			{
				Pose_Task_Update_Yaw(Yaw);

				if ((HAL_GetTick() - last_pose_display_ms) >=
				    POSE_DISPLAY_INTERVAL_MS)
				{
					last_pose_display_ms = HAL_GetTick();
					OLED_Update_Pose();
				}

				if ((HAL_GetTick() - last_pose_uart_ms) >=
				    POSE_UART_REPORT_INTERVAL_MS)
				{
					last_pose_uart_ms = HAL_GetTick();
					printf("Pitch:%.2f Roll:%.2f Yaw:%.2f\r\n",
					       Pitch, Roll, Yaw);
				}
			}
		}

    /* ===== B. USER 按键：只触发一次电池电压采样 ===== */
    if (Key_Proc() == 1U)
    {
			/* 启动 ADC1，对 PA3 对应的 ADC1_IN3 进行一次转换。 */
			if (HAL_ADC_Start(&hadc1) != HAL_OK)
			{
				printf("ADC START FAILED\r\n");
			}
			else if (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK)
			{
				/* 读取 12 位 ADC 结果，再按 1:1 分压电路还原电池实际电压。 */
				adc_value = (uint16_t)HAL_ADC_GetValue(&hadc1);
				voltage_value = ((float)adc_value / 4095.0f) * 3.3f * 2.0f + 0.02f;

				/* 同时输出原始值和电压，方便与万用表读数比较和校准。 */
				printf("Battery ADC:%u Voltage:%.2fV\r\n",
				       adc_value,
				       voltage_value);
			}
			else
			{
				/* 10 ms 内没有完成转换时给出提示，避免误把旧数据当成新结果。 */
				printf("ADC CONVERSION TIMEOUT\r\n");
			}

			/* 单次转换结束后关闭 ADC，等待下一次按键再次触发。 */
			(void)HAL_ADC_Stop(&hadc1);
		}

		/* ===== C. 串口命令：GO/STOP/LEFT/RIGHT/STATUS?/SPEED=xx ===== */
		if (USART3_Get_RxState() == 1U)
		{
			uint8_t speed_percent;
			uint8_t command_valid = 1U;

			if (strcmp((const char *)usart3_rxbuffer_pointer, "GO") == 0)
			{
				Pose_Task_Stop();
				color_control_armed = 1U;
				auto_enabled = 1U;
				turn_count = 0U;
				car_state = CAR_FORWARD;
			}
			else if (strcmp((const char *)usart3_rxbuffer_pointer, "STOP") == 0)
			{
				Pose_Task_Stop();
				color_control_armed = 0U;
				auto_enabled = 0U;
				turn_count = 0U;
				car_state = CAR_STOP;
			}
			else if (strcmp((const char *)usart3_rxbuffer_pointer, "LEFT") == 0)
			{
				Pose_Task_Stop();
				color_control_armed = 0U;
				auto_enabled = 0U;
				turn_count = 0U;
				car_state = CAR_LEFT;
			}
			else if (strcmp((const char *)usart3_rxbuffer_pointer, "RIGHT") == 0)
			{
				Pose_Task_Stop();
				color_control_armed = 0U;
				auto_enabled = 0U;
				turn_count = 0U;
				car_state = CAR_RIGHT;
			}
			else if (strcmp((const char *)usart3_rxbuffer_pointer, "STATUS?") == 0)
			{
			
			}
			else if (Parse_Speed_Command((const char *)usart3_rxbuffer_pointer,
			                             &speed_percent) == 1U)
			{
				car_speed_percent = speed_percent;
				car_speed_pwm = (uint16_t)(((uint32_t)PWM_PERIOD * car_speed_percent) / 100U);
			}
			else
			{
				uint16_t diagnostic_index;

				command_valid = 0U;
				printf("ERROR:COMMAND LEN:%u HEX:", usart3_rx_count);
				for (diagnostic_index = 0U;
				     diagnostic_index < usart3_rx_count;
				     diagnostic_index++)
				{
					printf("%02X", usart3_rx_buffer[diagnostic_index]);
				}
				printf("\r\n");
			}

			if (command_valid == 1U)
			{
				time_count = 0U;
				light_state = 1U;
				Car_Control();
				(void)HAL_UART_Transmit(&huart3,
				                       uart_direct_ack_message,
				                       sizeof(uart_direct_ack_message) - 1U,
				                       100U);
				UART_Send_Status();
			}

			USART3_Start_Receive();
		}

		/* ===== D. 50 ms 调度节拍：只负责清除节拍标志，并按开关调度超声波 ===== */
		if (control_tick_50ms == 1U)
		{
			__disable_irq();
			control_tick_50ms = 0U;
			__enable_irq();

#if ULTRASONIC_AVOIDANCE_ENABLE
			if ((auto_enabled == 1U) || (Pose_Task_Is_Active() == 1U))
			{
				measure_flag = 1U;
				if ((auto_enabled == 1U) &&
				    (car_state == CAR_AVOID_RIGHT) &&
				    (turn_count < AVOID_TURN_TICKS))
				{
					turn_count++;
				}
			}
#endif
		}

		/* ===== E. 超声波避障：当前由总开关关闭，不测距、不打印、不干预姿态旋转 ===== */
#if ULTRASONIC_AVOIDANCE_ENABLE
		if (measure_flag == 1U)
		{
			measure_flag = 0U;
			if (((auto_enabled == 1U) &&
			     ((car_state == CAR_FORWARD) || (car_state == CAR_STOP))) ||
			    (Pose_Task_Is_Active() == 1U))
			{
				HC_SR04Start();
				front_distance = HC_SR04Read();
				printf("DISTANCE:%.1f cm\r\n", front_distance);

				if (front_distance > 0.0f)
				{
					if (front_distance < OBSTACLE_DISTANCE_CM)
					{
						obstacle_present = 1U;
					}
					else if (front_distance >= OBSTACLE_RESUME_DISTANCE_CM)
					{
						obstacle_present = 0U;
					}
				}

				if ((pose_task_state == POSE_ROTATING) &&
				    (obstacle_present == 1U))
				{
					pose_task_state = POSE_PAUSED_BY_OBSTACLE;
					car_state = CAR_STOP;
				}
				else if ((auto_enabled == 1U) &&
				         (car_state == CAR_FORWARD) &&
				         (obstacle_present == 1U))
				{
					car_state = CAR_AVOID_RIGHT;
					turn_count = 0U;
					time_count = 0U;
					light_state = 1U;
					UART_Send_Status();
				}
			}
		}

		if ((auto_enabled == 1U) && (Pose_Task_Is_Active() == 0U))
		{
			if ((car_state == CAR_AVOID_RIGHT) &&
			    (turn_count >= AVOID_TURN_TICKS))
			{
				car_state = CAR_FORWARD;
				turn_count = 0U;
				time_count = 0U;
				light_state = 1U;
				UART_Send_Status();
			}
		}
		else if (Pose_Task_Is_Active() == 0U)
		{
			measure_flag = 0U;
			turn_count = 0U;
		}
#else
		/* 超声波避障当前关闭：持续清除残留状态，防止旧障碍标志让小车误停。 */
		measure_flag = 0U;
		obstacle_present = 0U;
		turn_count = 0U;
#endif

		/* ===== F. 姿态任务状态机：读取新 Yaw，完成每次 90 度旋转及 1 秒间隔 ===== */
		Pose_Task_Process();

		/* ===== G. 颜色识别：红色立即停车；绿色仅在姿态任务空闲时启动前进 ===== */
		/* 其他颜色保持上一次有效状态，不覆盖按键、串口或姿态任务的决定。 */
		if ((HAL_GetTick() - last_color_sample_ms) >= COLOR_SAMPLE_INTERVAL_MS)
		{
			uint8_t color_report_due;

			last_color_sample_ms = HAL_GetTick();
			color_report_due = ((HAL_GetTick() - last_color_uart_ms) >=
			                    COLOR_UART_REPORT_INTERVAL_MS) ? 1U : 0U;

			if (TCS34725_GetRawData(&rgb))
			{
				RGB_normalize(&rgb);
				if (color_report_due == 1U)
				{
					last_color_uart_ms = HAL_GetTick();
					printf("R=%d G=%d B=%d C=%d\r\n", rgb.r, rgb.g, rgb.b, rgb.c);
				}

				if ((rgb.c >= COLOR_CLEAR_MIN) &&
				    (rgb.r > rgb.g + COLOR_MARGIN) &&
				    (rgb.r > rgb.b + COLOR_MARGIN))
				{
					if (color_report_due == 1U)
					{
						printf("Red\r\n");
					}
					Pose_Task_Stop();
					auto_enabled = 0U;
					measure_flag = 0U;
					turn_count = 0U;
					car_state = CAR_STOP;
				}
				else if ((rgb.c >= COLOR_CLEAR_MIN) &&
				         (rgb.g > rgb.r + COLOR_MARGIN) &&
				         (rgb.g > rgb.b + COLOR_MARGIN))
				{
					if (color_report_due == 1U)
					{
						printf("Green\r\n");
					}

					/* 绿色只有在人工授权后才能启动前进，复位本身不构成授权。 */
					if ((color_control_armed == 1U) &&
					    (pose_task_state == POSE_IDLE))
					{
						auto_enabled = 1U;
						car_speed_percent = COLOR_FORWARD_SPEED_PERCENT;
						car_speed_pwm = (uint16_t)(((uint32_t)PWM_PERIOD *
						                            car_speed_percent) / 100U);
						if (car_state != CAR_AVOID_RIGHT)
						{
							turn_count = 0U;
							car_state = (obstacle_present == 1U) ?
							            CAR_STOP : CAR_FORWARD;
						}
					}
				}
				else if (color_report_due == 1U)
				{
					printf("Other color, keep state\r\n");
				}
			}
		}

		/* ===== H. 统一输出：更新运行标志、状态灯和电机，避免各模块直接争抢电机 ===== */
		start_flag = ((car_state != CAR_STOP) ||
		              (Pose_Task_Is_Active() == 1U)) ? 1U : 0U;
		Light_Blink_Proc();
		Car_Control();

		/* ===== I. 轮速采样：只用于观察和串口诊断，不参与本轮姿态闭环 ===== */
		if ((Speed_Measure_GetLeftDataState() == 1U) ||
		    (Speed_Measure_GetRightDataState() == 1U))
		{
			float left_motor_speed = Speed_Measure_GetLeftData();
			float right_motor_speed = Speed_Measure_GetRightData();

			/* 原地转向时内侧轮理论速度按 0 显示，避免把静止轮的残余计数误认为运动。 */
			if (car_state == CAR_LEFT)
			{
				left_motor_speed = 0.0f;
			}
			else if ((car_state == CAR_RIGHT) ||
			         (car_state == CAR_AVOID_RIGHT))
			{
				right_motor_speed = 0.0f;
			}
			else if (car_state == CAR_STOP)
			{
				left_motor_speed = 0.0f;
				right_motor_speed = 0.0f;
			}

			printf("LEFT_SPEED:%.2f r/s RIGHT_SPEED:%.2f r/s\r\n",
			       left_motor_speed, right_motor_speed);

			if (Speed_Measure_GetLeftDataState() == 1U)
			{
				Speed_Measure_LeftDataProcessComplt();
			}
			if (Speed_Measure_GetRightDataState() == 1U)
			{
				Speed_Measure_RightDataProcessComplt();
			}
		}
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
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
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART3)
    {   
        if (usart3_rx_temp == 0U)
        {
            /* Ignore stray NUL bytes from serial-terminal send buffers. */
            HAL_UART_Receive_IT(&huart3, &usart3_rx_temp, 1U);
        }
        else if ((usart3_rx_temp == '\r') || (usart3_rx_temp == '\n'))
        {
            if (usart3_rx_count > 0)
            {
                usart3_rx_buffer[usart3_rx_count] = '\0';
                usart3_rx_cmplt_flag = 1U;
            }
            else
            {
                HAL_UART_Receive_IT(&huart3, &usart3_rx_temp, 1U);
            }
        }
        else if (usart3_rx_count < (USART3_RX_BUFFER_NUM - 1U))
        {
            usart3_rx_buffer[usart3_rx_count] = usart3_rx_temp;
            usart3_rx_count++;
            usart3_rx_buffer[usart3_rx_count] = '\0';

            /* Accept the short control commands even when CR/LF is omitted. */
            if ((strcmp((const char *)usart3_rx_buffer, "GO") == 0) ||
                (strcmp((const char *)usart3_rx_buffer, "STOP") == 0) ||
                (strcmp((const char *)usart3_rx_buffer, "LEFT") == 0) ||
                (strcmp((const char *)usart3_rx_buffer, "RIGHT") == 0) ||
                (strcmp((const char *)usart3_rx_buffer, "STATUS?") == 0))
            {
                usart3_rx_cmplt_flag = 1U;
            }
            else
            {
                HAL_UART_Receive_IT(&huart3, &usart3_rx_temp, 1U);
            }

        }
        else
        {
            usart3_rx_buffer[usart3_rx_count] = '\0';
            usart3_rx_cmplt_flag = 1U;
        }
				
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if ((GPIO_Pin == MEMS_INT_Pin) && (init_flag == 1U))
    {
        mems_data_rdy_flag = 1U;
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
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
