#ifndef __MPU6050_H
#define __MPU6050_H

#include "stdint.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h" 
#include "MOTOR.h"
#include "Math.h"
#include "OLED.h"
extern I2C_HandleTypeDef hi2c2; 

#define MPU6050_ADDR            0xD0 

/* MPU6050 ÄÚ²¿¼Ä´æÆ÷µØÖ· */
#define SMPLRT_DIV              0x19
#define CONFIG                  0x1A
#define GYRO_CONFIG             0x1B
#define ACCEL_CONFIG            0x1C
#define ACCEL_XOUT_H            0x3B
#define TEMP_OUT_H              0x41
#define GYRO_XOUT_H             0x43
#define PWR_MGMT_1              0x6B
#define WHO_AM_I                0x75

/* Êý¾Ý½á¹¹Ìå */
typedef struct
{
    int16_t Accel_X;
    int16_t Accel_Y;
    int16_t Accel_Z;
    int16_t Temp;
    int16_t Gyro_X;
    int16_t Gyro_Y;
    int16_t Gyro_Z;
} MPU6050_T;

extern MPU6050_T g_tMPU6050;

/* º¯ÊýÉùÃ÷ */
void MPU6050_Init(void);
void MPU6050_ReadData(void);
uint8_t MPU6050_ReadID(void);
void MPU6050_Calibrate_Z(void);
void MPU6050_Turn_Angle(float target_angle, int speed);
void Car_Go_Straight_Gyro_Integration(int16_t Speed, uint32_t Time_ms,uint32_t sharpbrake_time);
#endif