#include "car_control.h"
#include "tb6612.h"

/* 数字循迹转向函数保留给自动掉头；PID 循迹使用下面单独的参数。 */
#define TRACKING_OUTER_SPEED 4500U
#define TRACKING_INNER_SPEED 2800U
#define TRACKING_STOP_SPEED     0U

/* 指导书第 11.3 节给出的第一组 PID 参数，先保持原值进行实车验证。 */
#define PID_KP          1.5f
#define PID_KI          0.05f
#define PID_KD          2.0f
#define PID_BASE_SPEED  3500
#define PID_MAX_SPEED   7200
#define PID_I_LIMIT     8000

static int32_t pid_last_error = 0;
static int32_t pid_integral = 0;
static int32_t pid_output = 0;

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
  error = (int32_t)right_adc - (int32_t)left_adc;

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

  left_speed = PID_BASE_SPEED + pid_output;
  right_speed = PID_BASE_SPEED - pid_output;

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
