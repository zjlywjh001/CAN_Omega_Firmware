/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "define.h"
#include "stm32f4xx_hal.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "stdio.h"
#include "mcp2515.h"
#include "led.h"
#include "tim.h"
#include "frontend.h"
#include "kwp2000.h"

#ifdef __GNUC__
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
	
	
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
volatile unsigned char state = STATE_CONFIG;
volatile unsigned char fuzz = 0;
volatile unsigned char fuzzpause = 0;
volatile unsigned char fuzzfrom[8];
volatile unsigned char fuzzto[8];
volatile unsigned char fuzzdata[8];
volatile unsigned char fuzzuntildata[8];
volatile unsigned char fuzzuntilmask;
volatile unsigned char comparelength;
volatile unsigned char fuzzdlc = 0;
volatile unsigned char fuzzextend = 0;
volatile int fuzzid = 0;
volatile int fuzzperiod = 0;
volatile unsigned char fuzzfin = 0;
volatile unsigned char fuzzprio = 0; //0 for lower first , 1 for higher first 
volatile unsigned char fuzzuntil = 0; //0 stop fuzzing until finish or manual, 1 stop fuzzing trigger by can receive message;
volatile unsigned int lastcounter;
u8 uart_recvdata = 0;
char line[LINE_MAXLEN];
unsigned char linepos = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
	MX_TIM5_Init();
	
	mcp2515_init();

  /* USER CODE BEGIN 2 */

	TXLED_OFF;
	RXLED_OFF;
	txledstatus = 0;
	rxledstatus = 0;
	
	HAL_TIM_Base_Stop(&htim5);
	HAL_TIM_Base_Stop(&htim3);
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_UART_Receive_IT(&huart1,&uart_recvdata,1);  
	//HAL_UART_Receive(&huart1, &uart_recvdata,1, 0xFFFF);
	canmsg_t canmsg_buffer[CANMSG_BUFFERSIZE];
	unsigned char canmsg_buffer_filled = 0;
	unsigned char canmsg_buffer_canpos = 0;
	unsigned char canmsg_buffer_usbpos = 0;
	unsigned char rxstep = 0;
	ResetFuzzer();
	
	//fuzz_test_setting();
	
	//printf("Hello World!\r\n");
  /* USER CODE END 2 */
	
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	u16 kbaud = 0;
	ISO9141Init(&kbaud);
	
  while (1)
  {
		//printf("Hello World!\r\n");
		HAL_UART_Receive_IT(&huart1,&uart_recvdata,1);  
		
		// handles interrupt requests of MCP2515 controller: receive message and store it to buffer
		if ((state != STATE_CONFIG) && (hardware_getMCP2515Int()) && (canmsg_buffer_filled < CANMSG_BUFFERSIZE)) 
		{
			if (mcp2515_receive_message(&canmsg_buffer[canmsg_buffer_canpos]))
			{
				canmsg_t recvmsg = canmsg_buffer[canmsg_buffer_canpos];
				if ((recvmsg.data[0]&0xF0)>>4==0x01)   //if a multi-frame message send flow control message
				{
					canmsg_t fcmsg;
					fcmsg.id = recvmsg.id;
					fcmsg.dlc = 3;
					fcmsg.flags.extended = recvmsg.flags.extended;
					fcmsg.data[0] = 0x30;
					for (int i=1;i<8;i++)
					{
						fcmsg.data[i] = 0;
					}
					while (mcp2515_send_message(&fcmsg)==0);
				}
				if (fuzz && fuzzuntil && (!fuzzpause) && (!fuzzfin)) 
				{
					int i;
					if (recvmsg.dlc>=comparelength) 
					{
						for (i = 0;i< recvmsg.dlc;i++) 
						{
							if (!(fuzzuntilmask&(1<<i)))
								continue;
							if (recvmsg.data[i]!=fuzzuntildata[i]) 
								break;
						}
						if (i==recvmsg.dlc) 
						{
							fuzzfin = 1;
							ResetFuzzer() ;
						}
					}
					
				}
				canmsg_buffer_canpos = (canmsg_buffer_canpos + 1) % CANMSG_BUFFERSIZE;
				canmsg_buffer_filled++;                
			}
    }
		
		while (canmsg_buffer_filled > 0)
		{
			printf("%c",canmsg2ascii_getNextChar(&canmsg_buffer[canmsg_buffer_usbpos], &rxstep));
			//canmsg2ascii_getNextChar(&canmsg_buffer[canmsg_buffer_usbpos], &rxstep);
			if (rxstep == RX_STEP_FINISHED) 
			{
					// finished this frame
					rxstep = 0;
					canmsg_buffer_usbpos = (canmsg_buffer_usbpos + 1) % CANMSG_BUFFERSIZE;
					canmsg_buffer_filled--;
			}
		}
		
		if (fuzz && (!fuzzpause) && (!fuzzfin)) 
		{
			int i;
			canmsg_t fuzzmsg;
			unsigned int nowtime = __HAL_TIM_GET_COUNTER(&htim5);
			int delta;
			if (nowtime<lastcounter) 
			{
				delta = nowtime+1000000-lastcounter;
			}
			else
			{
				delta = nowtime - lastcounter;
			}
			if (delta>=fuzzperiod) 
			{
				fuzzmsg.id = fuzzid;
				fuzzmsg.dlc = fuzzdlc;
				if (fuzzextend)
				{
					fuzzmsg.flags.extended = 1;
				}
				for (i = 0;i<fuzzdlc; i++) 
				{
					fuzzmsg.data[i] = fuzzdata[i];
				}
				while (mcp2515_send_message(&fuzzmsg)==0);
				sendFuzzProcess();
				lastcounter = __HAL_TIM_GET_COUNTER(&htim5);
				delta = 0;
				getNextFuzzData();
				if (fuzzfin)
				{
					ResetFuzzer() ;
				}
				
			}
			
		}
		
		if (kmsgpending)
		{
			int resplen = KWP2000_Fast_Transreceiver(sendmessage,msglen,recvmessage);
			if (resplen>0)
			{
				printf("%c",'k');
				for (int i=0;i<resplen;i++)
				{
					printf("%02x",recvmessage[i]);
				}
			}
			kmsgpending = 0;
		}
	

  }
		/* USER CODE END WHILE */
		
		
	/* USER CODE BEGIN 3 */
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 200;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * name   : u8 SPI1_TransferByte(u8 byte)
 * input  :	
 * return : void
 * desc.  : SPI1 trans
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
u8 SPI1_TransferByte(u8 byte)
{
	u8 data;
	
	HAL_SPI_TransmitReceive(&hspi1, (u8 *)&byte, (u8 *)&data, 1, 0xFFFF);
	
	return data;
}

void delay_us(u32 nus)
{
  u32 temp;
	SysTick->LOAD = 12*nus;
	SysTick->VAL=0x00;
	SysTick->CTRL=0x01;
	do
	{
		temp = SysTick->CTRL;
	} while ((temp&0x01)&&(!(temp&(1<<16))));
	SysTick->CTRL=0x00;
	SysTick->VAL=0x00;
}

/*
void fuzz_test_setting() 
{
	int i;
	fuzz = 1;
	fuzzid = 0x7DF;
	fuzzdlc = 3;
	fuzzprio = 1;
	fuzzpause = 0;
	fuzzperiod = 20000;
	fuzzfrom[0] = 0x02;
	fuzzfrom[1] = 0x01;
	fuzzfrom[2] = 0x00;
	fuzzuntilmask = 0xFE;
	comparelength = 0;
	for (i = 0;i<8;i++)
	{
		comparelength += ((fuzzuntilmask&(1<<i))!=0);
	}
	fuzzuntil = 0;
	//fuzzfrom[3] = 0x05;
	fuzzto[0] = 0x02;
	fuzzto[1] = 0x01;
	fuzzto[2] = 0xFF;
	//fuzzto[3] = 0x08;
	for (i = 0; i < fuzzdlc; i++)
	{
		fuzzdata[i] = fuzzfrom[i];
		fuzzuntildata[i] = 0;
	}
	fuzzuntildata[0] = 0x06;
	fuzzuntildata[1] = 0x41;
	fuzzuntildata[2] = 0x01;
	fuzzuntildata[3] = 0x00;
	fuzzuntildata[4] = 0x00;
	fuzzuntildata[5] = 0x00;
	fuzzuntildata[6] = 0x00;
	parseLine("O\r");
	HAL_TIM_Base_Start(&htim5);
	lastcounter = __HAL_TIM_GET_COUNTER(&htim5);
}*/

void getNextFuzzData() 
{
	int i;
	switch (fuzzprio)
	{
		case 0:
			if (fuzzdata[0]+0x01<=fuzzto[0]) 
			{
				fuzzdata[0] ++;
			} 
			else 
			{
				fuzzdata[0] = fuzzfrom[0];
				for (i = 1; i< fuzzdlc; i ++) 
				{
					if (fuzzfrom[i]==fuzzto[i])
						continue;
					if (fuzzdata[i]+1<=fuzzto[i]) 
					{
						fuzzdata[i]++;
						break;
					}
					else 
					{
						fuzzdata[i] = fuzzfrom[i];
					}
				}
				if (i==fuzzdlc) 
				{
					fuzzfin = 1;
				}
			}
			break;
		case 1:
			if (fuzzdata[fuzzdlc-1]+0x01<=fuzzto[fuzzdlc-1]) 
			{
				fuzzdata[fuzzdlc-1]++;
			} 
			else 
			{
				fuzzdata[fuzzdlc-1] = fuzzfrom[fuzzdlc-1];
				for (i = fuzzdlc-2; i>=0 ; i--) 
				{
					if (fuzzfrom[i]==fuzzto[i])
						continue;
					if (fuzzdata[i]+1<=fuzzto[i]) 
					{
						fuzzdata[i]++;
						break;
					}
					else 
					{
						fuzzdata[i] = fuzzfrom[i];
					}
				}
				if (i<0) 
				{
					fuzzfin = 1;
				}
			}
	}
	
}


/* USER CODE END 4 */
/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF); 
	flashTXLED();
	return ch;
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
