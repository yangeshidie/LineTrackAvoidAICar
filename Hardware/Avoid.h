#ifndef __AVOID_H
#define __AVOID_H

#include "MPU6050.h"
#include "MOTOR.h"
#include "LINE_TRACKER.h"
#include "cmsis_os.h"

extern osThreadId_t MotorConfigHandle; 

/* ================= 1. Òý½Å¶¨Òå (Çë¸ù¾ÝÊµ¼ÊÐÞ¸Ä) ================= */
// Trig (´¥·¢½Å): Output
#define HCSR04_TRIG_PORT    GPIOD
#define HCSR04_TRIG_PIN     GPIO_PIN_0

// Echo (»ØÏì½Å): Input
#define HCSR04_ECHO_PORT    GPIOD
#define HCSR04_ECHO_PIN     GPIO_PIN_1

/* ================= 2. ãÐÖµ¶¨Òå ================= */
#define OBSTACLE_DIST_CM    20.0f
#define OBSTACLE_AVOIDANCE_FLAG 0x0001U

/* ================= 3. º¯ÊýÉùÃ÷ ================= */
void Obstacle_Init(void);           // ³õÊ¼»¯ (Èç¹ûCubeMXÃ»ÅäGPIO£¬ÐèÔÚ´ËÅäÖÃ)
float HCSR04_Read_Distance(void);   // ¶ÁÈ¡¾àÀë
void Run_Obstacle_Avoidance(void);  // Ö´ÐÐ±ÜÕÏÈ«Á÷³Ì

#endif