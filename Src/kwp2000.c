
#include "define.h"
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "tim.h"
#include "kwp2000.h"

u8 KWP2000ProtocolVersion[2];

vu16 timer1 = 0;
vu16 timer2 = 0;
vu16 timerKW = 0;


u8  kwp2000_max_init_attempts = 2; 
u8  kwp2000_max_byte_transmit_attempts = 1;

u8  kwp2000_interbyte_delay = 2;
u8  kwp2000_interbyte_delaymax = 40;
u16 kwp2000_intermessage_delay = 10; 
u16 kwp2000_intermessage_delaymax = 500; 


/* ISO9141Init
     errors:
     0x10 - wrong protocol - not KWP2000 ketwords 0x01 0x8a
     0x7f - zla wartosc bajtu treningowego z  ECU (zla predkosc transmisji)
     0x8f - odczytany zostal inny bajt niz wyslany (halfduplex)
     0xff - timeout
*/
u8 ISO9141Init(u16 * uartSpeed) 
{
  u8 c, keyword;
  int i;
  u16 time, t2;
	
	USART2_Deinit();

	Init_5Baud(0x33);
	
//	TXDK(0);
//	TXDL(0);
//	delay_us(25000);
//	TXDK(1);
//	TXDL(1);
//  timer1=1000;

  if (0 == (*uartSpeed))
  {
    while ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)) && timer1) ;
    if (0 == timer1) 
    {
      return 0xff; 
    }
		
   
		__HAL_TIM_SET_COUNTER(&htim3,0);
		HAL_TIM_Base_Start(&htim3);
    //t2 = timer1;

    for (i=0; i<4; i++)
    {
      while ((0 == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)) && timer1) ;
      if (0 == timer1) 
      {
        return 0xff; 
      }
      while ((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)) && timer1); 
      if (0 == timer1) 
      {
        return 0xff; 
      }
    }

    while ((0 == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)) && timer1);
    if (0 == timer1) 
    {
      return 0xff; 
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
		
    timer1=2; //2ms
    while (timer1);
    HAL_UART_Receive(&huart2, &c, 1, 100);
  }
  else
  {
		t2 = timer1;
		
    MX_USART2_UART_Init(*uartSpeed);
		
    HAL_UART_Receive(&huart2, &c, 1, 100);
		
		if (HAL_UART_Receive(&huart2, &c, 1, t2)!=HAL_OK)
		{
			return 0xff; 
		}
 
    HAL_UART_Receive(&huart2, &c, 1, 100);
    if (0x55 != c) 
    {
      return 0x7f; //sync err 
    }
  }
  
  t2=500; // 0.5 sec
	
  if (HAL_UART_Receive(&huart2, &c, 1, t2)!=HAL_OK)
  {
    return 0xff; 
  }

  KWP2000ProtocolVersion[0] = c;

  if (HAL_UART_Receive(&huart2, &c, 1, t2)!=HAL_OK)
  {
    return 0xff; 
  }
  KWP2000ProtocolVersion[1] = c; 

	u8 tmp = 255 - KWP2000ProtocolVersion[1];
  HAL_UART_Transmit(&huart2,&tmp,1,0xFFFF);
  
  if (HAL_UART_Receive(&huart2, &c, 1, t2)!=HAL_OK)
  {
    return 0xff; 
  }

  if ((255-KWP2000ProtocolVersion[1]) != c) 
  {
    return 0x8f; // odczytany zostal inny bajt niz wyslany
  }
  
  if ((KWP2000ProtocolVersion[0] != 0x01) || (KWP2000ProtocolVersion[1] != 0x8A))
  {
    return 0x10;
  }

  timer1 = 0;
  return 0x00;
}

void Init_5Baud(u8 addr)
{
	int i;
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

/* KWP2000SendByteReceiveAnswer - wysyla bajt b oraz weryfikuje na zanegowana odpowiedz od ECU
     errors:
     0x00 - all ok
     0x1f; // nie odczytano wyslanego bajtu (przekroczony czas 2ms)
     0x2f; //odczytany zostal inny bajt niz wyslany (halfduplex)
     0x7f - bledne potwierdzenie bajtu od ecu
     0xff - trzy proby wyslania bajtu nieudane (przekroczony czas t2max
     
     kwp2000_max_byte_transmit_attempts = 3;
     kwp2000_interbyte_delaymax = 40; 
*/
u8 KWP2000SendByteReceiveAnswer (u8 b) 
{
  u8 c;
  u8 cnt;
  
  cnt = kwp2000_max_byte_transmit_attempts; // trzy proby
  while (cnt--)
  {
		HAL_UART_Transmit(&huart2,&b,1,0xFFFF);

//    timerKW = 2; //2ms na odebranie wyslanego bajtu
//    while (!USART2_DataAvailable() && timerKW);
//    if (0 == timerKW) 
//    {
//      return 0x1f; // nie odczytano wyslanego bajtu
//    }
		HAL_UART_Receive(&huart2, &c, 1, 0xFFFF);
    if (b != c) 
    {
      return 0x2f; //odczytany zostal inny bajt niz wyslany (halfduplex)
    }
  
    timerKW = kwp2000_interbyte_delaymax;
    if (HAL_UART_Receive(&huart2, &c, 1, timerKW)!=HAL_OK)
    {
      continue; // nie otrzymano zanegowanej odpowiedzi w przeciagu delay_kwp2000_t2max
    }
    if ((255-b) != c) 
    {
      return 0x7f; //bledne potwierdzenie bajtu od ecu
    }
  
    return 0x00; // all ok
  }
  return 0xff; // trzy proby nieudane
}

/* KWP2000SendBlock - wysyla blok danych do ecu
     errors:
     0x00 - all ok
     0x1f; // nie odczytano wyslanego bajtu (przekroczony czas 2ms)
     0x2f; //odczytany zostal inny bajt niz wyslany (halfduplex)
     
     u8 kwp2000_interbyte_delay = 2;
*/  
u8 KWP2000SendBlock(KWP2000struct_t * block)
{
  u8 i;
  u8 resp;

  resp = KWP2000SendByteReceiveAnswer(block->len); // block length
  if (resp > 0)
  {
    return resp;
  }
  
  for (i=0; i < kwp2000_interbyte_delay; i++)
  {
    delay_us(1000); //1ms
  }
  
  resp = KWP2000SendByteReceiveAnswer(block->cnt); //block counter
  if (resp > 0)
  {
    return resp;
  }

  for (i=0; i < kwp2000_interbyte_delay; i++)
  {
    delay_us(1000); //1ms
  }

  resp = KWP2000SendByteReceiveAnswer(block->title); //block title
  if (resp > 0)
  {
    return resp;
  }

  for (i=0; i < kwp2000_interbyte_delay; i++)
  {
    delay_us(1000); //1ms
  }

  if ( ((block->len) > 3) && (block->data != NULL) ) //block data
  {
    for (i=0; i < ((block->len)-3); i++) 
    {
      resp = KWP2000SendByteReceiveAnswer( *( block->data +i) ); //block data
      if (resp > 0)
      {
        return resp;
      }
      
      for (i=0; i < kwp2000_interbyte_delay; i++)
      {
        delay_us(1000); //1ms
      }
    }
  }
  
  u8 tmp=KWP2000_BLOCK_END; //block end
	HAL_UART_Transmit(&huart2,&tmp,1,0xFFFF);
  
//  timerKW = 2; //2ms na odebranie wyslanego bajtu
//  while (!USART2_DataAvailable() && timerKW);
//  if (0 == timerKW) 
//  {
//    return 0x1f; // nie odczytano wyslanego bajtu
//  }
  HAL_UART_Receive(&huart2, &resp, 1, 0xFFFF);
  if (KWP2000_BLOCK_END != resp) 
  {
    return 0x2f; //odczytany zostal inny bajt niz wyslany (halfduplex)
  }
  
  return 0x00; //wyslanie bloku zakonczylo sie powodzeniem
}


/* KWP2000ReceiveByteSendAnswer - odbiera bajt b oraz wysyla jego zanegowana wartosc do ECU
     errors:
     0x00 - all ok
     0x1e; // nie odczytano wyslanego bajtu (przekroczone 2ms)
     0x2e; //odczytany zostal inny bajt niz wyslany (halfduplex)
     0xde - brak odpowiedzi od ecu (przekroczony czas kwp2000_interbyte_delaymax)
     
     kwp2000_interbyte_delaymax = 40;
     kwp2000_interbyte_delay = 2;
*/
u8 KWP2000ReceiveByteSendAnswer (u8 * b) 
{
  u8 resp;
  u16 i;
  
  timerKW = kwp2000_interbyte_delaymax;
  if (HAL_UART_Receive(&huart2, b, 1, timerKW)!=HAL_OK)
  {
    return 0xde; //brak odpowiedzi od ecu
  }

  for (i=0; i < kwp2000_interbyte_delay; i++)
  {
    delay_us(1000); //1ms
  }
  
	u8 tmp = 255-(*b); // wyslanie negacji odebranego bajtu
	HAL_UART_Transmit(&huart2,&tmp,1,0xFFFF);
//  timerKW = 2;
//  while (!USART2_DataAvailable() && timerKW);
//  if (0 == timerKW) 
//  {
//    return 0x1e; 
//  }

  HAL_UART_Receive(&huart2, &resp, 1, 0xFFFF);
  
  if ((255-(*b)) != resp) {
    return 0x2e; // odczytany zostal inny bajt niz wyslany
  }
  
  return 0x00;
}

/* KWP2000ReceiveBlock - odebranie bloku z ecu

errors:
    0x00 - all ok
    0xdd - brak odpowiedzi od ecu (poczatek ramki), przekroczony t3max, mozna sprobowac ponownie wyslac komunikat
    0xed - bledna wartosc bajtu konczacego blok 
    0xfd - nie przyszedl bajt konczacy blok

u16 kwp2000_intermessage_delaymax = 500;
*/
u8 KWP2000ReceiveBlock(KWP2000struct_t * block) 
{
  u8 i;
  u8 resp;
  
  timerKW = kwp2000_intermessage_delaymax;
  
  resp = KWP2000ReceiveByteSendAnswer( &(block->len) ); //odebranie Block Length
  if (resp > 0) 
  {
    return resp;
  }

  resp = KWP2000ReceiveByteSendAnswer( &(block->cnt) ); //odebranie Block Counter
  if (resp > 0) 
  {
    return resp;
  }
 
  resp = KWP2000ReceiveByteSendAnswer( &(block->title) );
  if (resp > 0) 
  {
    return resp;
  }
  
  if ( (block->len) > 3 ) //block data
  {
    for (i=0; i < ((block->len)-3); i++) 
    {
      resp = KWP2000ReceiveByteSendAnswer( block->data + i); //block data
      if (resp > 0) 
      {
        return resp;
      }
    }
  }

  timerKW = kwp2000_interbyte_delaymax;
  if (HAL_UART_Receive(&huart2, &i, 1, timerKW)!=HAL_OK)
  {
    return 0xfd; 
  }
	
  if (0x03 != i)
  {
    return 0xed;
  }

  return 0x00; //wyslanie bloku zakonczylo sie powodzeniem
}

