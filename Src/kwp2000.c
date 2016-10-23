
#include "define.h"
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "tim.h"
#include "kwp2000.h"

u8 KWP2000ProtocolVersion[2];

vu16 timer1 = 0;
vu16 timer2 = 0;
vu16 timerKW = 0;
int p3_timer = 0;
u8 sendkl=0;


u8  kwp2000_max_init_attempts = 2; 
u8  kwp2000_max_byte_transmit_attempts = 1;

u8  kwp2000_interbyte_delay = 2;
u8  kwp2000_interbyte_delaymax = 40;
u16 kwp2000_intermessage_delay = 10; 
u16 kwp2000_intermessage_delaymax = 500; 

u8 recvmessage[256];
u8 sendmessage[256];
u8 k_state = K_UNKNOWN;


/* ISO9141Init
     errors:
     0x10 - wrong protocol - not ISO9141-2 ketwords 0x08 0x08
     0x7f - zla wartosc bajtu treningowego z  ECU (zla predkosc transmisji)
     0x8f - odczytany zostal inny bajt niz wyslany (halfduplex)
     0xff - timeout
*/
u8 ISO9141Init(u16 * uartSpeed)
{
  u8 c;
  int i;
  u16 time;
	u8 ok,synctime;
	ok = 0;
	c=0;
	synctime = 0;
	
	KWP2000_Fast_Init();

	delay_us(500000);
	delay_us(500000);
	delay_us(500000);

	while ((!ok) && synctime<kwp2000_max_init_attempts) 
	{
		synctime ++;
	
		
		Init_5Baud(0x33);
		
		timer1=1000;

		if (0 == (*uartSpeed))
		{
			while ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == SET) && timer1) ;  //waiting for start bit
			if (0 == timer1) 
			{
				if (synctime>=kwp2000_max_init_attempts) 
				{
					return 0xff; 
				}
				else 
				{
					continue;
				}
			}
			
			__HAL_TIM_SET_COUNTER(&htim3,0);
			HAL_TIM_Base_Start(&htim3);
			//t2 = timer1;
			

			for (i=0; i<4; i++)
			{
				while ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == RESET) && timer1) ; 
				if (0 == timer1) 
				{
					if (synctime>=kwp2000_max_init_attempts) 
					{
						return 0xff; 
					}
					else 
					{
						continue;
					}
				}
				while ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)==SET) && timer1); 
				if (0 == timer1) 
				{
					if (synctime>=kwp2000_max_init_attempts) 
					{
						return 0xff; 
					}
					else 
					{
						continue;
					}
				}
			}

			while ((RESET == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)) && timer1);
			if (0 == timer1) 
			{
				if (synctime>=kwp2000_max_init_attempts) 
				{
					return 0xff; 
				}
				else 
				{
					continue;
				}
			}

			time = __HAL_TIM_GET_COUNTER(&htim3);
			HAL_TIM_Base_Stop(&htim3);
			
			if (time > 900)
			{
				*uartSpeed = 9600;
			}
			else 
			{
				*uartSpeed = 10400;
			}

			MX_USART2_UART_Init(*uartSpeed);
			c = huart2.Instance->DR & 0x00FFU;  //rubbish
		}
		else
		{
			MX_USART2_UART_Init(*uartSpeed);
			
			c = huart2.Instance->DR & 0x00FFU; //rubbish
			
			while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) && timer1);
			if (0 == timer1) 
			{
				if (synctime>=kwp2000_max_init_attempts) 
				{
					return 0xff; 
				}
				else 
				{
					continue;
				}
			}		
			c = huart2.Instance->DR & 0x00FFU;
	 
			if (0x55 != c) 
			{
				return 0x7f; //sync error
			}
			
		}
		
		timer1=500; // 0.5 sec
		
		while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) && timer1);
		if (0 == timer1) 
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0xff; 
			}
			else 
			{
				continue;
			}
		}
		KWP2000ProtocolVersion[0] = huart2.Instance->DR & 0x00FFU;
		
		while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) && timer1);
		if (0 == timer1) 
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0xff; 
			}
			else 
			{
				continue;
			}
		}

		KWP2000ProtocolVersion[1] = huart2.Instance->DR & 0x00FFU;

		u8 tmp = 0xFF - KWP2000ProtocolVersion[1];
		
		HAL_UART_Transmit(&huart2,&tmp,1,0xFFFF);
		
		while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) && timer1);
		if (0 == timer1) 
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0xff; 
			}
			else 
			{
				continue;
			}
		}
		c = huart2.Instance->DR & 0x00FFU;

		if ((0xFF-KWP2000ProtocolVersion[1]) != c) 
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0x8f; 
			}
			else 
			{
				continue;
			}
		}
		
		//Wrong Protocol 		
		if ((KWP2000ProtocolVersion[0] != 0x08) || (KWP2000ProtocolVersion[1] != 0x08))
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0x10; 
			}
			else 
			{
				continue;
			}
		}
	
		ok = 1;
	}
	
	

  timer1 = 0;
  return 0x00;
}

void KWP2000_Fast_Init()
{
	
	USART2_Deinit();

	TXDK(0);
	TXDL(0);
	delay_us(25000);
	TXDK(1);
	TXDL(1);
	MX_USART2_UART_Init(10400);
	delay_us(25000);
	
}

/* ISO9141Init
     errors:
     0x10 - wrong protocol - not KWP2000_5Baud ketwords 0xE9 0x8F
     0x7f - zla wartosc bajtu treningowego z  ECU (zla predkosc transmisji)
     0x8f - odczytany zostal inny bajt niz wyslany (halfduplex)
     0xff - timeout
*/
u8 KWP2000_5Baud_Init()
{
  u8 c;
	u8 ok,synctime;
	ok = 0;
	c=0;
	synctime = 0;
	
	KWP2000_Fast_Init();
			
	
	delay_us(500000);
	delay_us(500000);
	delay_us(500000);
	
	while ((!ok) && synctime<kwp2000_max_init_attempts) 
	{
		synctime ++;

		Init_5Baud(0x33);
		
		timer1=1000;
		
		MX_USART2_UART_Init(10400);
		
		c = huart2.Instance->DR & 0x00FFU; //rubbish
		
		while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) && timer1);
		if (0 == timer1) 
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0xff; 
			}
			else 
			{
				continue;
			}
		}		
		c = huart2.Instance->DR & 0x00FFU;
 
		if (0x55 != c) 
		{
			return 0x7f; //sync error
		}

		timer1=500; // 0.5 sec
		
		while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) && timer1);
		if (0 == timer1) 
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0xff; 
			}
			else 
			{
				continue;
			}
		}
		KWP2000ProtocolVersion[0] = huart2.Instance->DR & 0x00FFU;
		
		while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) && timer1);
		if (0 == timer1) 
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0xff; 
			}
			else 
			{
				continue;
			}
		}

		KWP2000ProtocolVersion[1] = huart2.Instance->DR & 0x00FFU;

		u8 tmp = 0xFF - KWP2000ProtocolVersion[1];
		
		HAL_UART_Transmit(&huart2,&tmp,1,0xFFFF);
		
		while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) && timer1);
		if (0 == timer1) 
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0xff; 
			}
			else 
			{
				continue;
			}
		}
		c = huart2.Instance->DR & 0x00FFU;

		if ((0xFF-KWP2000ProtocolVersion[1]) != c) 
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0x8f; 
			}
			else 
			{
				continue;
			}
		}
		
		//Wrong Protocol 		
		if ((KWP2000ProtocolVersion[0] != 0xE9) || (KWP2000ProtocolVersion[1] != 0x8F))
		{
			if (synctime>=kwp2000_max_init_attempts) 
			{
				return 0x10; 
			}
			else 
			{
				continue;
			}
		}
	
		ok = 1;
	}

  timer1 = 0;
  return 0x00;
}


void KWP2000_SendMsg(u8 *msg,int len)
{
	HAL_UART_Transmit(&huart2,msg,len,0xFFFF);
}

u8 KWP2000_RecvMsg(u8* recvbuffer)
{
	int i;
	
	i = huart2.Instance->DR & 0x00FFU;
	
	i = 0;
	timer1 = 100;
	while (1)
	{
		while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) && timer1);
		if (timer1==0) 
		{
			break;
		} 
		recvbuffer[i] = huart2.Instance->DR & 0x00FFU;
		timer1 = 100;
		i++;
		if (i>=256)
			break;
	}
	
	return i;
	
	
}



void Init_5Baud(u8 addr)
{
	int i;
	USART2_Deinit();
	
	delay_us(300000);
	
	TXDK(0); //start bit
	TXDL(0);
	delay_us(200000);
	
	for (i =0;i<8;i++)
	{
		TXDK((addr&(1<<i))!=0);
		TXDL((addr&(1<<i))!=0);
		delay_us(200000);
	}
	
	
	TXDK(1); //stop bit
	TXDL(1);
	
	delay_us(200000);
	
}

u8 checksum(u8* data,u8 len)
{
	u8 i;
	u8 sum;
	sum = 0;
	for (i=0;i<len;i++)
	{
		sum = sum + data[i];
	}
	return sum;
}


