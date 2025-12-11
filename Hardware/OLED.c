#include "OLED.h"
#include "OLED_Font.h"

const uint8_t Init_Command[]=
{
0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0xA1,0xC8,
0xDA,0x12,0x81,0xCF,0xD9,0xF1,0xDB,0x30,0xA4,0xA6,
0x8D,0x14,0xAF
};

void OLED_C(uint8_t cmd)
{
	HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,&cmd,1,0x100);
}

void OLED_D(uint8_t data)
{
	HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x40,I2C_MEMADD_SIZE_8BIT,&data,1,0x100);
}

void OLED_SetCursor(uint8_t Y,uint8_t X)
{
	OLED_C(0xB0 | Y);
	OLED_C(0x00 | (X & 0x0F)); //È¡XµÍËÄÎ»
	OLED_C(0x10 | ((X & 0xF0)>>4));//È¡X¸ßËÄÎ»
}

void OLED_DispChar(uint8_t Row,uint8_t Col,char Char,uint8_t Size)
{
	if(Size==Size8x16)
	{	
		OLED_SetCursor((Row-1)*2,(Col-1)*8);
		for(uint8_t i=0;i<8;i++)
		{
			OLED_D(OLED_F8x16[Char - ' '][i]);
		}
		OLED_SetCursor((Row-1)*2+1,(Col-1)*8);
		for(uint8_t i=0;i<8;i++)
		{
			OLED_D(OLED_F8x16[Char - ' '][i+8]);
		}
	}
//		else if(Size==Size6x8)
//	{
//	
//	}
}

void OLED_DispString(uint8_t Row,uint8_t Col,char Char[],uint8_t Size)
{
	uint8_t n=strlen(Char);
	for(uint8_t i=0;i<n;i++){
		OLED_DispChar(Row,Col+i,Char[i],Size);
	}
}
void OLED_DispUNum(uint8_t Row, uint8_t Col, uint32_t Num, uint8_t Size)
{
    uint8_t len = 0;
    char CharNum[20]; // 缓冲区

    // 1. 特殊情况：如果数字是0，直接显示并返回
    if (Num == 0)
    {
        OLED_DispChar(Row, Col, '0', Size);
        return;
    }

    // 2. 提取数字（此时存入的是倒序的，例如123 -> CharNum中是['3','2','1']）
    while (Num > 0)
    {
        CharNum[len] = (Num % 10) + '0'; // 取个位
        Num /= 10;                       // 去掉个位
        len++;
    }

    // 3. 倒序遍历数组进行显示（即修正为正序）
    // 注意：j 定义为 int8_t 或者使用下面的写法防止 uint8_t 死循环
    for (uint8_t i = 0; i < len; i++)
    {
        // i 是显示的第几个字符（0, 1, 2...）
        // len - 1 - i 是数组中对应的倒序索引
        
        /* 
           假设 Size 是字宽/间距。
           如果你的 Col 是指像素坐标，这里可能需要 Col + i * (Size/2) 之类的。
           如果你的 Col 是指字符格数，则直接 Col + i。
           这里沿用你原代码的逻辑 Col + i。
        */
        OLED_DispChar(Row, Col + i, CharNum[len - 1 - i], Size);
    }
}

void OLED_DispSNum(uint8_t Row,uint8_t Col,int32_t Num,uint8_t Size)
{	

	if(Num>0){OLED_DispChar(Row,Col,'+',Size);}
	else if(Num==0){OLED_DispChar(Row,Col+1,'0',Size);}
	else{OLED_DispChar(Row,Col,'-',Size);Num=-Num;}
		
	OLED_DispUNum(Row,Col+1,(uint32_t)Num,Size);
}


void OLED_Clear(void)
{
	for(int i=0;i<8;i++)
	{
		OLED_SetCursor(i,0);
		for(int j=0;j<128;j++)
		{
			OLED_D(0x00);
		}
	}
}

void OLED_Init(void)
{
	//HAL_Delay(200);
	
	for(uint8_t i=0;i<23;i++)
	{
		OLED_C(Init_Command[i]);
	}
	
	OLED_Clear();
}

void OLED_Display(OLEDConfigStr* ptrToMessage)
{
    OLED_Clear();
    OLED_DispString(1, 1, "Red:", 8);
    OLED_DispString(2, 1, "Green:", 8);
    OLED_DispString(3, 1, "Debug:", 8);
    OLED_DispUNum(1, 9, ptrToMessage->red, 3);
    OLED_DispUNum(2, 9, ptrToMessage->green, 3);
    OLED_DispUNum(3, 9, ptrToMessage->debugData, 3);

    return;
}
