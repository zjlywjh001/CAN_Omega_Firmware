#ifndef _KWP2000_H
#define _KWP2000_H

#include "define.h"

#define KWP2000_ACK 0x09
#define KWP2000_NO_ACK 0x0A
#define KWP2000_END_OF_SESSION 0x06
#define KWP2000_ASCII 0xF6
#define KWP2000_0x02_FRAME 0x02

#define KWP2000_ERRORS_DELETE 0x05
#define KWP2000_ERRORS_REQ 0x07
#define KWP2000_ERRORS_RESP 0xFC

#define KWP2000_GROUP_REQ 0x29
#define KWP2000_GROUP_RESP 0xE7

#define KWP2000_BLOCK_END 0x03

extern vu16 timer1;
extern vu16 timer2;
extern vu16 timerKW;
extern vu8 buttonState;

extern u8  kwp2000_max_init_attempts; 
extern u8  kwp2000_max_byte_transmit_attempts; 

extern u8  kwp2000_interbyte_delay;
extern u8  kwp2000_interbyte_delaymax;
extern u16 kwp2000_intermessage_delay;
extern u16 kwp2000_intermessage_delaymax;

typedef struct {
  u8 len;
  u8 cnt;
  u8 title;
  u8 * data;
} KWP2000struct_t;

extern u8 KWP2000ProtocolVersion[2];

u8 ISO9141Init(u16 * uartSpeed);
u8 KWP2000SendBlock(KWP2000struct_t * block);
u8 KWP2000ReceiveBlock(KWP2000struct_t * block);
void Init_5Baud(u8 addr);

#endif
