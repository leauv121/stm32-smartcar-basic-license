#include "car_control.h"

#include "config/car_parameters.h"
#include "control/line_follow_control.h"
#include "control/motion_control.h"
#include "services/safety_service.h"

static int32_t pid_output;
static char line_following_mode = 'N';

static void CarControl_Apply(int16_t left, int16_t right)
{
  MotionRequest request;
  request.mode = ((left == 0) && (right == 0)) ? MOTION_STOP : MOTION_DRIVE;
  request.left_target = left;
  request.right_target = right;
  request = SafetyService_Resolve(request, 0U, 0U);
  MotionControl_Apply(request);
}

static void CarControl_ApplyLineOutput(LineFollowOutput output)
{
  pid_output = output.pid_output;
  if (output.mode == LINE_FOLLOW_RESCUE_RIGHT)
  {
    line_following_mode = 'R';
  }
  else if (output.mode == LINE_FOLLOW_RESCUE_LEFT)
  {
    line_following_mode = 'L';
  }
  else
  {
    line_following_mode = 'N';
  }
  CarControl_Apply(output.left_target, output.right_target);
}

/**
  * @brief  小车强左转，仅供掉头等需要内轮停止的动作使用
  */
void Smart_Car_Turn_Left(void)
{
  CarControl_Apply(0, CAR_LINE_RESCUE_OUTER_SPEED);
}

/**
  * @brief  小车弱左转，参数与当前循迹救线控制器保持一致
  */
void Smart_Car_Turn_Left_Weak(void)
{
  CarControl_Apply(CAR_LINE_RESCUE_INNER_SPEED,
                   CAR_LINE_RESCUE_OUTER_SPEED);
}

/**
  * @brief  小车强右转，仅供掉头等需要内轮停止的动作使用
  */
void Smart_Car_Turn_Right(void)
{
  CarControl_Apply(CAR_LINE_RESCUE_OUTER_SPEED, 0);
}

/**
  * @brief  小车弱右转，参数与当前循迹救线控制器保持一致
  */
void Smart_Car_Turn_Right_Weak(void)
{
  CarControl_Apply(CAR_LINE_RESCUE_OUTER_SPEED,
                   CAR_LINE_RESCUE_INNER_SPEED);
}

void Smart_Car_Go_Straight(void)
{
  CarControl_Apply(CAR_LINE_RESCUE_OUTER_SPEED,
                   CAR_LINE_RESCUE_OUTER_SPEED);
}

void Smart_Car_Go_Straight_Fast(void)
{
  CarControl_Apply(6000, 6000);
}

void Smart_Car_Stop(void)
{
  CarControl_Apply(0, 0);
}

/**
  * @brief  兼容旧接口，实际计算统一委托给新循迹控制器
  * @note   不再维护第二套 PID 状态，避免参数和滤波行为发生分叉。
  */
void Line_Following_PID(uint16_t left_adc, uint16_t right_adc)
{
  LineFollowInput input = {left_adc, right_adc, 0U, 0U, 1U};
  CarControl_ApplyLineOutput(LineFollowControl_Update(input));
}

void Line_Following_PID_Reset(void)
{
  LineFollowControl_Reset();
  pid_output = 0;
  line_following_mode = 'N';
}

void Line_Following_PID_Prime(uint16_t left_adc, uint16_t right_adc)
{
  LineFollowControl_Prime(left_adc, right_adc);
  pid_output = 0;
  line_following_mode = 'N';
}

/**
  * @brief  兼容旧四路混合循迹接口
  * @note   LINE1 为外右 PA6，LINE4 为外左 PB1；高电平表示黑线。
  */
void Line_Following_Hybrid(uint16_t left_adc, uint16_t right_adc)
{
  LineFollowInput input;

  input.left_adc = left_adc;
  input.right_adc = right_adc;
  input.outer_right = (uint8_t)(HAL_GPIO_ReadPin(LINE1_GPIO_Port,
                                                LINE1_Pin) == GPIO_PIN_SET);
  input.outer_left = (uint8_t)(HAL_GPIO_ReadPin(LINE4_GPIO_Port,
                                               LINE4_Pin) == GPIO_PIN_SET);
  input.sample_ready = 1U;
  CarControl_ApplyLineOutput(LineFollowControl_Update(input));
}

char Line_Following_Get_Mode(void)
{
  return line_following_mode;
}

int32_t Line_Following_PID_GetOutput(void)
{
  return pid_output;
}
