#include "car_control.h"
#include "tb6612.h"

/* 四路循迹模块从右向左依次保存为 LINE1、LINE2、LINE3、LINE4。 */
GPIO_PinState front_line[4] = {GPIO_PIN_RESET};

/*
 * 基础循迹实车调参：
 * 直行和转弯外侧轮使用 4500；弱转弯内侧轮保留 2800，不再直接停轮，
 * 用连续的双轮差速代替“走一下、停一下”，减小中间位置反复修正时的顿挫。
 */
#define TRACKING_OUTER_SPEED 4500U
#define TRACKING_INNER_SPEED 2800U
#define TRACKING_STOP_SPEED     0U

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
  * @brief  按指导书第 11.2 节读取四路 DO 电平并执行基础循迹
  * @param  None
  * @retval None
  * @note   LINE1 到 LINE4 按照车头朝前时从右到左排列。
  *         指导书规定 GPIO_PIN_SET 表示对应模块检测到黑线。
  */
void Smart_Car_Automatic_Tracing(void)
{
  /* 先一次性读取四路输入，保证本轮判断使用同一时刻附近的数据。 */
  front_line[0] = HAL_GPIO_ReadPin(LINE1_GPIO_Port, LINE1_Pin);
  front_line[1] = HAL_GPIO_ReadPin(LINE2_GPIO_Port, LINE2_Pin);
  front_line[2] = HAL_GPIO_ReadPin(LINE3_GPIO_Port, LINE3_Pin);
  front_line[3] = HAL_GPIO_ReadPin(LINE4_GPIO_Port, LINE4_Pin);

  /*
   * 实车验证补充的居中状态：
   * 最右 LINE1 和最左 LINE4 位于白面，中间 LINE2、LINE3 同时压住黑线时，
   * 说明黑线处于车体中央，应优先直行，避免弱左/弱右交替让车速变慢。
   */
  if ((front_line[0] == GPIO_PIN_RESET) &&
      (front_line[1] == GPIO_PIN_SET) &&
      (front_line[2] == GPIO_PIN_SET) &&
      (front_line[3] == GPIO_PIN_RESET))
  {
    Smart_Car_Go_Straight();
  }
  else if ((front_line[0] == GPIO_PIN_SET) &&
      ((front_line[2] == GPIO_PIN_RESET) ||
       (front_line[3] == GPIO_PIN_RESET)))
  {
    /* 从右数第一个模块检测到黑线：小车强右转。 */
    Smart_Car_Turn_Right();
  }
  else if ((front_line[1] == GPIO_PIN_SET) &&
           ((front_line[2] == GPIO_PIN_RESET) ||
            (front_line[3] == GPIO_PIN_RESET)))
  {
    /* 从右数第二个模块检测到黑线：小车弱右转。 */
    Smart_Car_Turn_Right_Weak();
  }
  else if ((front_line[3] == GPIO_PIN_SET) &&
           ((front_line[0] == GPIO_PIN_RESET) ||
            (front_line[1] == GPIO_PIN_RESET)))
  {
    /* 从右数第四个模块检测到黑线：小车强左转。 */
    Smart_Car_Turn_Left();
  }
  else if ((front_line[2] == GPIO_PIN_SET) &&
           ((front_line[0] == GPIO_PIN_RESET) ||
            (front_line[1] == GPIO_PIN_RESET)))
  {
    /* 从右数第三个模块检测到黑线：小车弱左转。 */
    Smart_Car_Turn_Left_Weak();
  }
  else
  {
    /* 没有出现需要纠偏的组合时，按照指导书保持直行。 */
    Smart_Car_Go_Straight();
  }
}
