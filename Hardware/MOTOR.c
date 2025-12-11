#include "MOTOR.h"

// 假设您的TIM9的自动重装载值 (ARR) 是 999，那么PWM的占空比范围就是 0-999。
// 这个宏用于将 0-100 的速度值映射到 0-ARR 的比较值。
// 请根据您的TIM9配置修改 ARR_VALUE。
#define ARR_VALUE 9999
#define CCR_CALCULATOR(speed) ((uint32_t)(speed * (ARR_VALUE+1) / 100))

// 内部函数，用于设置两个电机的速度和方向
static void robot_speed(int left_speed, int right_speed);

/**
 * @brief 启动电机PWM输出
 */
void Motor_Start(void)
{
    // 启动TIM9的两个PWM通道
    HAL_TIM_PWM_Start(&htim9, MOTOR_LEFT_PWM_CHANNEL);
    HAL_TIM_PWM_Start(&htim9, MOTOR_RIGHT_PWM_CHANNEL);
    
    // 初始状态，电机停止
    __HAL_TIM_SET_COMPARE(&htim9, MOTOR_LEFT_PWM_CHANNEL, 0);
    __HAL_TIM_SET_COMPARE(&htim9, MOTOR_RIGHT_PWM_CHANNEL, 0);
}

/**
 * @brief 设置左右轮速度和方向的核心函数
 * @param left_speed  左轮速度 (-100 到 100, 负数表示后退)
 * @param right_speed 右轮速度 (-100 到 100, 负数表示后退)
 */
static void robot_speed(int left_speed, int right_speed)
{
    // 左轮方向和速度控制
    if (left_speed >= 0) {
        HAL_GPIO_WritePin(LEFT_MOTOR_IN1_PORT, LEFT_MOTOR_IN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LEFT_MOTOR_IN2_PORT, LEFT_MOTOR_IN2_PIN, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim9, MOTOR_LEFT_PWM_CHANNEL, CCR_CALCULATOR(left_speed));
    } else {
        HAL_GPIO_WritePin(LEFT_MOTOR_IN1_PORT, LEFT_MOTOR_IN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LEFT_MOTOR_IN2_PORT, LEFT_MOTOR_IN2_PIN, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim9, MOTOR_LEFT_PWM_CHANNEL, CCR_CALCULATOR(-left_speed));
    }

    // 右轮方向和速度控制
    if (right_speed >= 0) {
        HAL_GPIO_WritePin(RIGHT_MOTOR_IN3_PORT, RIGHT_MOTOR_IN3_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(RIGHT_MOTOR_IN4_PORT, RIGHT_MOTOR_IN4_PIN, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim9, MOTOR_RIGHT_PWM_CHANNEL, CCR_CALCULATOR(right_speed));
    } else {
        HAL_GPIO_WritePin(RIGHT_MOTOR_IN3_PORT, RIGHT_MOTOR_IN3_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RIGHT_MOTOR_IN4_PORT, RIGHT_MOTOR_IN4_PIN, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim9, MOTOR_RIGHT_PWM_CHANNEL, CCR_CALCULATOR(-right_speed));
    }
}


/**
 * @brief 小车前进
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Run(int8_t speed, uint16_t time)
{
    if (speed > 100) speed = 100;
    if (speed < 0) speed = 0;
    
    robot_speed(speed, speed);
    HAL_Delay(time); // 使用HAL库的延时函数
}

/**
 * @brief 小车后退
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Back(int8_t speed, uint16_t time)
{
    if (speed > 100) speed = 100;
    if (speed < 0) speed = 0;

    robot_speed(-speed, -speed);
    HAL_Delay(time);
    robot_speed(0, 0); // 运行结束后停止
}

/**
 * @brief 小车刹车
 * @param time 持续时间 (毫秒)
 */
void Car_Brake(uint16_t time)
{
    robot_speed(0, 0);
    HAL_Delay(time);
}

/**
 * @brief 小车左旋转 (原地)
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Spin_Left(int8_t speed, uint16_t time)
{
    if (speed > 100) speed = 100;
    if (speed < 0) speed = 0;

    robot_speed(-speed, speed); // 左轮后退，右轮前进
    HAL_Delay(time);
    robot_speed(0, 0); // 运行结束后停止
}

/**
 * @brief 小车右旋转 (原地)
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Spin_Right(int8_t speed, uint16_t time)
{
    if (speed > 100) speed = 100;
    if (speed < 0) speed = 0;

    robot_speed(speed, -speed); // 左轮前进，右轮后退
    HAL_Delay(time);
    robot_speed(0, 0); // 运行结束后停止
}

/**
 * @brief 小车左转 (差速)
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Left(int8_t speed, uint16_t time)
{
    if (speed > 100) speed = 100;
    if (speed < 0) speed = 0;

    // 左轮慢，右轮快，实现左转
    robot_speed(speed / 2, speed); 
    HAL_Delay(time);
    robot_speed(0, 0);
}

/**
 * @brief 小车右转 (差速)
 * @param speed 速度 (0-100)
 * @param time 持续时间 (毫秒)
 */
void Car_Right(int8_t speed, uint16_t time)
{
    if (speed > 100) speed = 100;
    if (speed < 0) speed = 0;

    // 左轮快，右轮慢，实现右转
    robot_speed(speed, speed / 2);
    HAL_Delay(time);
    robot_speed(0, 0);
}
/* @brief 设置左右轮速度和方向 (无延时)
* @param left_speed  左轮速度 (-100 到 100, 负数表示后退)
* @param right_speed 右轮速度 (-100 到 100, 负数表示后退)
*/
void Car_Set_Speed(int left_speed, int right_speed)
{
    // 对速度进行限幅，防止超出 -100 到 100 的范围
    if (left_speed > 100)  left_speed = 100;
    if (left_speed < -100) left_speed = -100;
    if (right_speed > 100) right_speed = 100;
    if (right_speed < -100) right_speed = -100;

    robot_speed(left_speed, right_speed);
}