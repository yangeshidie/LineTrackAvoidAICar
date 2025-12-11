#ifndef __MOTOR_H
#define __MOTOR_H
#include "stdint.h"
#include "stm32f4xx_hal.h"



// 根据您的实际接线修改GPIO端口和引脚
#define LEFT_MOTOR_IN1_PORT      GPIOB
#define LEFT_MOTOR_IN1_PIN       GPIO_PIN_12
#define LEFT_MOTOR_IN2_PORT      GPIOB
#define LEFT_MOTOR_IN2_PIN       GPIO_PIN_13

#define RIGHT_MOTOR_IN3_PORT     GPIOB
#define RIGHT_MOTOR_IN3_PIN      GPIO_PIN_14
#define RIGHT_MOTOR_IN4_PORT     GPIOB
#define RIGHT_MOTOR_IN4_PIN      GPIO_PIN_15

// 引用在main.c中定义的TIM_HandleTypeDef
extern TIM_HandleTypeDef htim9;

// 定义PWM通道
#define MOTOR_LEFT_PWM_CHANNEL   TIM_CHANNEL_1
#define MOTOR_RIGHT_PWM_CHANNEL  TIM_CHANNEL_2

/**
 * @brief 电机启动函数
 */
void Motor_Start(void);

/**
 * @brief 小车前进
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Run(int8_t speed, uint16_t time);

/**
 * @brief 小车后退
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Back(int8_t speed, uint16_t time);

/**
 * @brief 小车刹车
 * @param time 持续时间 (毫秒)
 */
void Car_Brake(uint16_t time);

/**
 * @brief 小车左旋转 (原地)
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Spin_Left(int8_t speed, uint16_t time);

/**
 * @brief 小车右旋转 (原地)
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Spin_Right(int8_t speed, uint16_t time);

/**
 * @brief 小车左转 (大半径)
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Left(int8_t speed, uint16_t time);

/**
 * @brief 小车右转 (大半径)
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Right(int8_t speed, uint16_t time);
/**
 * @brief 设置左右轮速度和方向 (无延时)
 * @param left_speed  左轮速度 (-100 到 100, 负数表示后退)
 * @param right_speed 右轮速度 (-100 到 100, 负数表示后退)
 */
void Car_Set_Speed(int left_speed, int right_speed);
#endif // __MOTOR_H

