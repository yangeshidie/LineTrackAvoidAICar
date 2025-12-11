#ifndef __OLED_H
#define __OLED_H

#include "stm32f4xx_hal.h"
#include "stdint.h"
#include "cstring"

#define Size8x16 		0
#define Size6x8 		1

typedef struct
{
  /* data */
  uint8_t red;
  uint8_t green;
  uint16_t debugData;
}
OLEDConfigStr;

extern I2C_HandleTypeDef hi2c1;

void OLED_C(uint8_t cmd);
void OLED_D(uint8_t data);
void OLED_SetCursor(uint8_t X,uint8_t Y);
void OLED_Display(OLEDConfigStr* ptrToMessage);

void OLED_DispChar(uint8_t Row,uint8_t Col,char Char,uint8_t Size);
void OLED_DispString(uint8_t Row,uint8_t Col,char Char[],uint8_t Size);
void OLED_DispUNum(uint8_t Row,uint8_t Col,uint32_t Char,uint8_t Size);
void OLED_DispSNum(uint8_t Row,uint8_t Col,int32_t Char,uint8_t Size);

void OLED_Clear(void);
void OLED_Init(void);

#endif
