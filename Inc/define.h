//
//  define.h
//  
//
//
//

#ifndef _DEFINE_H
#define _DEFINE_H

#include "stm32f4xx.h"
#include "stdio.h"


/*!< STM32F10x Standard Peripheral Library old types (maintained for legacy purpose) */
typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef const int32_t sc32;  /*!< Read Only */
typedef const int16_t sc16;  /*!< Read Only */
typedef const int8_t sc8;   /*!< Read Only */

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;  /*!< Read Only */
typedef __I int16_t vsc16;  /*!< Read Only */
typedef __I int8_t vsc8;   /*!< Read Only */

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef const uint32_t uc32;  /*!< Read Only */
typedef const uint16_t uc16;  /*!< Read Only */
typedef const uint8_t uc8;   /*!< Read Only */

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;  /*!< Read Only */
typedef __I uint16_t vuc16;  /*!< Read Only */
typedef __I uint8_t vuc8;   /*!< Read Only */

#define STATE_CONFIG 0
#define STATE_OPEN 1
#define STATE_LISTEN 2
#define VERSION_HARDWARE_MAJOR 0
#define VERSION_HARDWARE_MINOR 1
#define VERSION_FIRMWARE_MAJOR 0
#define VERSION_FIRMWARE_MINOR 1


#define CANMSG_BUFFERSIZE 8

#define STATE_CONFIG 0
#define STATE_OPEN 1
#define STATE_LISTEN 2

#define hardware_getMCP2515Int() HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13)==GPIO_PIN_RESET

extern volatile unsigned char state;
extern volatile unsigned char fuzz;
extern volatile unsigned char fuzzpause;
extern volatile unsigned char fuzzfrom[8];
extern volatile unsigned char fuzzto[8];
extern volatile unsigned char fuzzdata[8];
extern volatile unsigned char fuzzuntildata[8];
extern volatile unsigned char fuzzuntilmask;
extern volatile unsigned char comparelength;
extern volatile unsigned char fuzzdlc;
extern volatile int fuzzid;
extern volatile int fuzzperiod;
extern volatile unsigned char fuzzfin;
extern volatile unsigned char fuzzprio; //0 for lower first , 1 for higher first 
extern volatile unsigned char fuzzuntil; //0 stop fuzzing until finish or manual, 1 stop fuzzing trigger by can receive message;
extern volatile unsigned int lastcounter;
extern volatile unsigned char fuzzextend;


void delay_us(u32 nus);
u8 SPI1_TransferByte(u8 byte);
//void fuzz_test_setting(void);
void getNextFuzzData(void);
void ResetFuzzer(void);
void TXDK(u8 state);
void TXDL(u8 state);
void USART2_Deinit(void);
#endif
