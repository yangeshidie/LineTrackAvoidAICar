#ifndef __SG90_H
#define __SG90_H

#include "stdint.h"
#include "stm32f4xx_hal.h"

#define SG90_TIM TIM4
#define SG90_CHANNEL1 TIM_CHANNEL_1
#define SG90_CHANNEL2 TIM_CHANNEL_2
#define DEFAULT_ANGLE 90
#define CCR_CALCULATOR(x) ((1000 + x * 3500 / 180) - 1) 

extern TIM_HandleTypeDef htim4;

enum SG90X
{
	SG901 = 1,
	SG902 = 2
};

typedef struct
{
  /* data */
  enum SG90X SG90x;
  uint8_t angle;
}
SG90ConfigStr;

void SG90_Start(void);
void SG90_Config(enum SG90X SG90x ,uint8_t angle);
#endif
