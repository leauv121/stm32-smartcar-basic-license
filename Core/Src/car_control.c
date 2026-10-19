#include "car_control.h"
#include "tb6612.h"

/**
  * @brief  小车左转
  * @param  None
  * @retval None
  */
void Smart_Car_Turn_Left(void)
{
  TB6612_SetSpeed(MOTOR_LEFT,0,0);
  TB6612_SetSpeed(MOTOR_RIGHT,0,3800);
}

/**
  * @brief  小车慢速左转
  * @param  None
  * @retval None
  */
void Smart_Car_Turn_Left_Weak(void)
{
  TB6612_SetSpeed(MOTOR_LEFT,0,0);
  TB6612_SetSpeed(MOTOR_RIGHT,0,3500);
}

/**
  * @brief  小车右转
  * @param  None
  * @retval None
  */
void Smart_Car_Turn_Right(void)
{
  TB6612_SetSpeed(MOTOR_LEFT,0,3800);
  TB6612_SetSpeed(MOTOR_RIGHT,0,0);
}

/**
  * @brief  小车慢速右转
  * @param  None
  * @retval None
  */
void Smart_Car_Turn_Right_Weak(void)
{
  TB6612_SetSpeed(MOTOR_LEFT,0,3500);
  TB6612_SetSpeed(MOTOR_RIGHT,0,0);
}

/**
  * @brief  小车直线行驶
  * @param  None
  * @retval None
  */
void Smart_Car_Go_Straight(void)
{
  TB6612_SetSpeed(MOTOR_LEFT,0,4000);
  TB6612_SetSpeed(MOTOR_RIGHT,0,4000);
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
  * @brief  小车快速停止行驶
  * @param  None
  * @retval None
  */
void Smart_Car_Stop(void)
{
  TB6612_SetSpeed(MOTOR_LEFT,0,0);
  TB6612_SetSpeed(MOTOR_RIGHT,0,0);
}
