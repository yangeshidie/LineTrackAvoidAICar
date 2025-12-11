#include "usart.h"

static _RXBUFF USART2_Rx_Attri;
static _RXBUFF rxMessage;
static _RXBUFF  CurrentRxBufStructure;
static uint8_t MV_CurrentFrame[FRAMESIZE];
_RXBUFF* pToCurrentRxBufStructure = &CurrentRxBufStructure;				//using for pointing to the rx buffer structure
uint8_t Global_RxBuffer[USART_BUFFER_SIZE]; 														//global USART buffer
extern osMessageQueueId_t MVQueueHandle;



void USART2_Buffer_Init(void)
{
	USART2_Rx_Attri.rp = 0;
	USART2_Rx_Attri.wp = 0;
	USART2_Rx_Attri.len = USART2_Rx_Attri.wp - USART2_Rx_Attri.rp;
}

 /**
  * @brief  Activate USART2_RX DMA
  * @param  Pointer to buffer
  * @retval None
  */
void USART_USER_DMA_USART2RX_START(uint8_t* pToMessageBuffer)
{
	HAL_UART_Receive_DMA(&huart2, pToMessageBuffer, USART_BUFFER_SIZE);
}

/**
  * @brief  Activate USART2_RX DMA
  * @param  addrOfData	----- Address of buffer
						size				----- Size of data
  * @retval None
  */
void USART_USER_DMA_USART2TX_TRANSMIT(uint8_t* addrOfData, uint16_t size)
{
	HAL_UART_Transmit_DMA(&huart2, addrOfData, size);
}

/**
  * @brief  Serial debug interface
  * @param  addrOfData	----- Address of buffer
						size				----- Size of data
  * @retval None
  */
void DEBUG_USART_TRANSMIT(uint8_t num)
{
	HAL_UART_Transmit(&huart2, &num, 1, 100);
}

/**
  * @brief  Used by USART2 IDLE interrupt, getting the length of the frame and sending a message
  * @param  None
  * @retval None
  */
void USART2_IDLEInterrup_Handler(void)
{
	uint32_t tempNum = 0;
	tempNum = __HAL_DMA_GET_COUNTER(&hdma_usart2_rx);
	USART2_Rx_Attri.wp = USART_BUFFER_SIZE - tempNum;
	
	
	//Continuous data
	if(USART2_Rx_Attri.wp > USART2_Rx_Attri.rp)
		USART2_Rx_Attri.len = USART2_Rx_Attri.wp - USART2_Rx_Attri.rp;
	//Be truncated
	if(USART2_Rx_Attri.wp < USART2_Rx_Attri.rp)
		USART2_Rx_Attri.len = USART_BUFFER_SIZE - USART2_Rx_Attri.rp + USART2_Rx_Attri.wp;
	rxMessage.rp = USART2_Rx_Attri.rp;
	rxMessage.wp = USART2_Rx_Attri.wp;
	rxMessage.len = USART2_Rx_Attri.len;
	rxMessage.u8 = 0;
	
	USART2_Rx_Attri.rp = USART2_Rx_Attri.wp;
	
	osMessageQueuePut(MVQueueHandle, &rxMessage, 2, 0);
}

/**
  * @brief  Get a frame form rx buffer
  * @param  pToAttri 				----- Pointer to a frame specific block
  * @retval successfulFLag  ----- 0/1
  */
uint8_t USART_FrameProcess(_RXBUFF* pToAttri)
{
	//Continuous data
	if(pToAttri->rp < pToAttri->wp)
	{
		uint16_t i = pToAttri->rp;
		uint16_t j = 0;
		for(; i < pToAttri->wp; i++, j++)
		{
			if(j >= FRAMESIZE)
				return 0;
			MV_CurrentFrame[j] = Global_RxBuffer[i];	
		}
	}
	
	//Partition data
	else if(pToAttri->rp > pToAttri->wp)
	{
		uint16_t i = pToAttri->rp;
		uint16_t j = 0;
		for(; i < USART_BUFFER_SIZE; i++, j++)
		{
			MV_CurrentFrame[j] = Global_RxBuffer[i];
		}
		
		for(i = 0; i < pToAttri->wp; i++, j++)
		{
			MV_CurrentFrame[j] = Global_RxBuffer[i];
		}
	}
	
	else
		return 0;
	
	return 1;
}

/**
  * @brief  Get message from current frame
  * @param  None
  * @retval successfulFLag  ----- 0/1
  */
uint8_t USART_VerifyDatafromFrame(_RXBUFF* pToAttri)
{
	if(strncmp((const char*)MV_CurrentFrame, "$MVDATA", 7) == 0)
	{
		//Valid data frame
		return 1;
	}
	else if (strncmp((const char*)MV_CurrentFrame, "$MVERR", 7) == 0)
	{
		
	}
	else if (strncmp((const char*)MV_CurrentFrame, "$MVERR", 7) == 0)
	{

	}
}
