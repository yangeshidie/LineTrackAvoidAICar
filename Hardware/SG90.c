#include "SG90.h"
//1000-4500  0-180
//angle*3500/180

void SG90_Start(void)
{
    __HAL_TIM_SET_COMPARE(&htim4, SG90_CHANNEL1, CCR_CALCULATOR(DEFAULT_ANGLE));
    __HAL_TIM_SET_COMPARE(&htim4, SG90_CHANNEL2, CCR_CALCULATOR(DEFAULT_ANGLE));
    HAL_TIM_PWM_Start(&htim4, SG90_CHANNEL1);
    HAL_TIM_PWM_Start(&htim4, SG90_CHANNEL2);
    return;
}

void SG90_Config(enum SG90X SG90x , uint8_t angle)
{
	switch (SG90x)
	{
		case SG901:
            __HAL_TIM_SET_COMPARE(&htim4, SG90_CHANNEL1, CCR_CALCULATOR(angle));
            HAL_TIM_PWM_Start(&htim4, SG90_CHANNEL1);
            break;
		case SG902:
            __HAL_TIM_SET_COMPARE(&htim4, SG90_CHANNEL2, CCR_CALCULATOR(angle));
            HAL_TIM_PWM_Start(&htim4, SG90_CHANNEL2);
            break;
        default:
            break;
	}
    return;
}