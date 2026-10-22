#include "car_control.h"
#include "tb6612.h"

/* 数字循迹转向函数保留给自动掉头；PID 循迹使用下面单独的参数。 */
#define TRACKING_OUTER_SPEED 4500U
#define TRACKING_INNER_SPEED 2800U
#define TRACKING_STOP_SPEED     0U

/*
 * 指导书第11.3节说明PID参数需要根据实车调整。
 * 第一轮以稳定为主：减弱积分和微分，避免直线段反复摆动。
 */
#define PID_KP                 1.0f
#define PID_KI                 0.0f
#define PID_KD                 0.5f
#define PID_START_SPEED        1800
#define PID_BASE_SPEED         3700
#define PID_MAX_SPEED          6500
#define PID_I_LIMIT            8000
#define PID_ERROR_DEADBAND       60
#define PID_SOFT_START_STEP      60

static int32_t pid_last_error = 0;
static int32_t pid_integral = 0;
static int32_t pid_output = 0;
static int32_t pid_base_speed = PID_START_SPEED;
static uint8_t pid_primed = 0U;
static char line_following_mode = 'N';

/**
  * @brief  计算带中心死区的双路模拟传感器偏差
  * @param  left_adc  左内侧传感器ADC值
  * @param  right_adc 右内侧传感器ADC值
  * @retval 处理后的偏差，正负方向与指导书保持一致
  */
static int32_t Line_Following_Get_Error(uint16_t left_adc, uint16_t right_adc)
{
  int32_t error = (int32_t)right_adc - (int32_t)left_adc;

  if ((error <= PID_ERROR_DEADBAND) &&
      (error >= -PID_ERROR_DEADBAND))
  {
    error = 0;
  }

  return error;
}

/**
  * @brief  从外侧急弯控制切回PID时同步历史误差
  * @note   保留当前基础速度，避免每次出弯都重新慢速启动。
  */
static void Line_Following_PID_Resume(uint16_t left_adc, uint16_t right_adc)
{
  pid_last_error = Line_Following_Get_Error(left_adc, right_adc);
  pid_integral = 0;
  pid_output = 0;
  pid_primed = 1U;
}

/**
  * @brief  小车强左转
  * @param  None
  * @retval None
  */
void Smart_Car_Turn_Left(void)
{
  TB6612_SetSpeed(MOTOR_LEFT, 0, TRACKING_STOP_SPEED);
  TB6612_SetSpeed(MOTOR_RIGHT, 0, TRACKING_OUTER_SPEED);
}

/**
  * @brief  小车弱左转
  * @param  None
  * @retval None
  */
void Smart_Car_Turn_Left_Weak(void)
{
  /* 弱左转时两轮继续前进，只让左轮慢于右轮。 */
  TB6612_SetSpeed(MOTOR_LEFT, 0, TRACKING_INNER_SPEED);
  TB6612_SetSpeed(MOTOR_RIGHT, 0, TRACKING_OUTER_SPEED);
}

/**
  * @brief  小车强右转
  * @param  None
  * @retval None
  */
void Smart_Car_Turn_Right(void)
{
  TB6612_SetSpeed(MOTOR_LEFT, 0, TRACKING_OUTER_SPEED);
  TB6612_SetSpeed(MOTOR_RIGHT, 0, TRACKING_STOP_SPEED);
}

/**
  * @brief  小车弱右转
  * @param  None
  * @retval None
  */
void Smart_Car_Turn_Right_Weak(void)
{
  /* 弱右转时两轮继续前进，只让右轮慢于左轮。 */
  TB6612_SetSpeed(MOTOR_LEFT, 0, TRACKING_OUTER_SPEED);
  TB6612_SetSpeed(MOTOR_RIGHT, 0, TRACKING_INNER_SPEED);
}

/**
  * @brief  小车直线行驶
  * @param  None
  * @retval None
  */
void Smart_Car_Go_Straight(void)
{
  TB6612_SetSpeed(MOTOR_LEFT, 0, TRACKING_OUTER_SPEED);
  TB6612_SetSpeed(MOTOR_RIGHT, 0, TRACKING_OUTER_SPEED);
}

/**
  * @brief  小车快速直线行驶
  * @param  None
  * @retval None
  */
void Smart_Car_Go_Straight_Fast(void)
{
  TB6612_SetSpeed(MOTOR_LEFT,0,6000);
  TB6612_SetSpeed(MOTOR_RIGHT,0,6000);
}

/**
  * @brief  小车停止
  * @param  None
  * @retval None
  */
void Smart_Car_Stop(void)
{
  TB6612_SetSpeed(MOTOR_LEFT,0,0);
  TB6612_SetSpeed(MOTOR_RIGHT,0,0);
}

/**
  * @brief  使用两路 AO 采样值执行指导书第 11.3 节 PID 循迹
  * @param  left_adc  左内侧传感器 ADC 值（PB0/ADC1_IN8）
  * @param  right_adc 右内侧传感器 ADC 值（PA7/ADC1_IN7）
  * @retval None
  */
void Line_Following_PID(uint16_t left_adc, uint16_t right_adc)
{
  int32_t error;
  int32_t left_speed;
  int32_t right_speed;

  /* 指导书规定偏差为“右侧采样值减左侧采样值”。 */
  error = Line_Following_Get_Error(left_adc, right_adc);

  /* 防止未显式预置时首次计算产生很大的微分冲击。 */
  if (pid_primed == 0U)
  {
    pid_last_error = error;
    pid_primed = 1U;
  }

  /* 积分限幅用于避免长时间偏向一侧时积分无限累积。 */
  pid_integral += error;
  if (pid_integral > PID_I_LIMIT)
  {
    pid_integral = PID_I_LIMIT;
  }
  else if (pid_integral < -PID_I_LIMIT)
  {
    pid_integral = -PID_I_LIMIT;
  }

  pid_output = (int32_t)(PID_KP * (float)error +
                         PID_KI * (float)pid_integral +
                         PID_KD * (float)(error - pid_last_error));
  pid_last_error = error;

  /* 启动后逐步提高基础速度，避免电机从静止瞬间跳到正常速度。 */
  if (pid_base_speed < PID_BASE_SPEED)
  {
    pid_base_speed += PID_SOFT_START_STEP;
    if (pid_base_speed > PID_BASE_SPEED)
    {
      pid_base_speed = PID_BASE_SPEED;
    }
  }

  left_speed = pid_base_speed + pid_output;
  right_speed = pid_base_speed - pid_output;

  if (left_speed > PID_MAX_SPEED)
  {
    left_speed = PID_MAX_SPEED;
  }
  else if (left_speed < 0)
  {
    left_speed = 0;
  }

  if (right_speed > PID_MAX_SPEED)
  {
    right_speed = PID_MAX_SPEED;
  }
  else if (right_speed < 0)
  {
    right_speed = 0;
  }

  TB6612_SetSpeed(MOTOR_LEFT, 0U, (uint16_t)left_speed);
  TB6612_SetSpeed(MOTOR_RIGHT, 0U, (uint16_t)right_speed);
}

/**
  * @brief  清除 PID 历史，防止停止、掉头或方向切换后沿用旧误差
  * @param  None
  * @retval None
  */
void Line_Following_PID_Reset(void)
{
  pid_last_error = 0;
  pid_integral = 0;
  pid_output = 0;
  pid_base_speed = PID_START_SPEED;
  pid_primed = 0U;
  line_following_mode = 'N';
}

/**
  * @brief  使用启动瞬间的传感器偏差预置PID，消除第一次微分突跳
  * @param  left_adc  左内侧传感器ADC值
  * @param  right_adc 右内侧传感器ADC值
  * @retval None
  */
void Line_Following_PID_Prime(uint16_t left_adc, uint16_t right_adc)
{
  int32_t error = Line_Following_Get_Error(left_adc, right_adc);

  pid_last_error = error;
  pid_integral = 0;
  pid_output = 0;
  pid_base_speed = PID_START_SPEED;
  pid_primed = 1U;
  line_following_mode = 'N';
}

/**
  * @brief  四传感器混合循迹：外侧DO救援急弯，中间AO执行连续PID
  * @param  left_adc  左内侧PB0/ADC1_IN8模拟值
  * @param  right_adc 右内侧PA7/ADC1_IN7模拟值
  * @note   LINE1为外右PA6，LINE4为外左PB1；高电平表示检测到黑线。
  */
void Line_Following_Hybrid(uint16_t left_adc, uint16_t right_adc)
{
  GPIO_PinState outer_right;
  GPIO_PinState outer_left;
  char previous_mode = line_following_mode;

  outer_right = HAL_GPIO_ReadPin(LINE1_GPIO_Port, LINE1_Pin);
  outer_left = HAL_GPIO_ReadPin(LINE4_GPIO_Port, LINE4_Pin);

  if ((outer_right == GPIO_PIN_SET) &&
      (outer_left == GPIO_PIN_RESET))
  {
    /* 外右压到黑线，说明弯道已超出中间PID范围，优先强右转。 */
    line_following_mode = 'R';
    Smart_Car_Turn_Right();
  }
  else if ((outer_left == GPIO_PIN_SET) &&
           (outer_right == GPIO_PIN_RESET))
  {
    /* 外左压到黑线，优先强左转，把黑线送回中间两个传感器之间。 */
    line_following_mode = 'L';
    Smart_Car_Turn_Left();
  }
  else
  {
    line_following_mode = 'N';
    if (previous_mode != 'N')
    {
      /* 救援结束后同步当前误差，避免微分项因模式切换产生突跳。 */
      Line_Following_PID_Resume(left_adc, right_adc);
    }
    Line_Following_PID(left_adc, right_adc);
  }
}

/**
  * @brief  返回当前循迹来源，供低频串口诊断
  * @retval N=中间PID，R=外右急弯救援，L=外左急弯救援
  */
char Line_Following_Get_Mode(void)
{
  return line_following_mode;
}

/**
  * @brief  获取最近一次 PID 输出，仅用于低频串口诊断
  * @param  None
  * @retval 最近一次左右轮差速控制量
  */
int32_t Line_Following_PID_GetOutput(void)
{
  return pid_output;
}
