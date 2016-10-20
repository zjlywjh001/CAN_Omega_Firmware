#ifndef _FRONTEND_H
#define _FRONTEND_H

#include "mcp2515.h"

#define LINE_MAXLEN 100
#define BELL 7
//#define CR 13
#define LR 10

#define RX_STEP_TYPE 0
#define RX_STEP_ID_EXT 1
#define RX_STEP_ID_STD 6
#define RX_STEP_DLC 9
#define RX_STEP_DATA 10
#define RX_STEP_TIMESTAMP 26
#define RX_STEP_CR 30
#define RX_STEP_FINISHED 0xff

extern unsigned char kmsgpending;
extern unsigned char msglen;
extern unsigned char pauseflag;
extern unsigned char stopflag;

unsigned char transmitStd(char *line);
void parseLine(char * line);
char canmsg2ascii_getNextChar(canmsg_t * canmsg, unsigned char * step);
void sendFuzzProcess(void);
unsigned char config_fuzzer(char *config);
void sendFuzzProcess(void);
u8 KWP2000FastMessage(char* data);
u8 J1850VPWMessage(char* data);
u8 J1850PWMMessage(char* data);

#endif
