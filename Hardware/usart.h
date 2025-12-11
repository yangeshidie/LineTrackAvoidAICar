#ifndef __USART_H
#define __USART_H

#include "main.h"
#include "string.h"
#include "cmsis_os.h"

#define SERIAL_USART USART2
#define USART_BUFFER_SIZE 500
#define MAX_FRAME 100
#define FRAMESIZE 8

typedef struct __RXSTRUCTBUFF
{
	uint16_t rp;			//Head Pointer
	uint16_t wp;			//Tail Pointer
	uint16_t len;		  //Length
	uint8_t u8;			  //Not in use
}_RXBUFF;

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern _RXBUFF* pToCurrentRxBufStructure;

void USART2_Buffer_Init(void);
void USART_USER_DMA_USART2RX_START(uint8_t* pToMessageBuffer);
void USART_USER_DMA_USART2TX_TRANSMIT(uint8_t* addrOfData, uint16_t size);
void DEBUG_USART_TRANSMIT(uint8_t num);
void USART2_IDLEInterrup_Handler(void);
uint8_t USART_FrameProcess(_RXBUFF* pToAttri);
uint8_t USART_VerifyDatafromFrame(_RXBUFF* pToAttri);

#endif