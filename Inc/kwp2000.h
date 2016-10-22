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

#define K_UNKNOWN 0
#define K_ISO9141_ACTIVE 1 
#define K_KWP2000_FAST_ACTIVE 2
#define K_KWP2000_5BAUD_ACTIVE 3
#define P3_TIMEOUT 4000

extern vu16 timer1;
extern vu16 timer2;
extern vu16 timerKW;
extern int p3_timer;

extern u8  kwp2000_max_init_attempts; 
extern u8  kwp2000_max_byte_transmit_attempts; 

extern u8  kwp2000_interbyte_delay;
extern u8  kwp2000_interbyte_delaymax;
extern u16 kwp2000_intermessage_delay;
extern u16 kwp2000_intermessage_delaymax;
extern u8 k_state;
extern u8 recvmessage[256];
extern u8 sendmessage[256];

typedef struct {
  u8 len;
  u8 cnt;
  u8 title;
  u8 * data;
} KWP2000struct_t;

extern u8 KWP2000ProtocolVersion[2];
extern u8 sendkl;

u8 ISO9141Init(u16 * uartSpeed);
u8 KWP2000_5Baud_Init(void);
u8 KWP2000SendBlock(KWP2000struct_t * block);
u8 KWP2000SendByteReceiveAnswer (u8 b) ;
u8 KWP2000ReceiveBlock(KWP2000struct_t * block);
void KWP2000_SendMsg(u8 *msg,int len);
u8 KWP2000_RecvMsg(u8* recvbuffer);
void Init_5Baud(u8 addr);
void KWP2000_Fast_Init(void);
u8 checksum(u8* data,u8 len);

#endif
