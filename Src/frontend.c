/********************************************************************
 File: frontend.c

 Description:
 This file contains the frontend interface functions.

 Authors and Copyright:
 (c) 2012-2016, Thomas Fischl <tfischl@gmx.de>

 Device: PIC18F14K50
 Compiler: Microchip MPLAB XC8 C Compiler V1.34

 License:
 This file is open source. You can use it or parts of it in own
 open source projects. For closed or commercial projects you have to
 contact the authors listed above to get the permission for using
 this source code.

 ********************************************************************/

#include "define.h"
#include "led.h"
#include "mcp2515.h"
#include "tim.h"
#include "frontend.h"
#include "kwp2000.h"
#include "j1850.h"

unsigned char timestamping = 0;
unsigned char kmsgpending = 0;
unsigned char msglen = 0;
unsigned char pauseflag = 0;
unsigned char stopflag = 0;
unsigned char activatetask = 0;

/**
 * Parse hex value of given string
 *
 * @param line Input string
 * @param len Count of characters to interpret
 * @param value Pointer to variable for the resulting decoded value
 * @return 0 on error, 1 on success
 */
unsigned char parseHex(char * line, unsigned char len, unsigned long * value) 
{
    *value = 0;
    while (len--) {
        if (*line == 0) return 0;
        *value <<= 4;
        if ((*line >= '0') && (*line <= '9')) {
           *value += *line - '0';
        } else if ((*line >= 'A') && (*line <= 'F')) {
           *value += *line - 'A' + 10;
        } else if ((*line >= 'a') && (*line <= 'f')) {
           *value += *line - 'a' + 10;
        } else return 0;
        line++;
    }
    return 1;
}

/**
 * Send given value as hexadecimal string
 *
 * @param value Value to send as hex over the UART
 * @param len Count of characters to produce
 */
void sendHex(unsigned long value, unsigned char len) 
{

    char s[20];
    s[len] = 0;

    while (len--) {

        unsigned char hex = value & 0x0f;
        if (hex > 9) hex = hex - 10 + 'A';
        else hex = hex + '0';
        s[len] = hex;

        value = value >> 4;
    }

    printf("%s",s);

}

/**
 * Send given byte value as hexadecimal string
 *
 * @param value Byte value to send over UART
 */
void sendByteHex(unsigned char value) 
{

//    sendHex(value, 2);
    
    unsigned char ch = value >> 4;
    if (ch > 9) ch = ch - 10 + 'A';
    else ch = ch + '0';
    printf("%c",ch);

    ch = value & 0xF;
    if (ch > 9) ch = ch - 10 + 'A';
    else ch = ch + '0';
    printf("%c",ch);
    
}

/**
 * Interprets given line and transmit can message
 *
 * @param line Line string which contains the transmit command
 */
unsigned char transmitStd(char *line) 
{
    canmsg_t canmsg;
    unsigned long temp;
    unsigned char idlen;

    canmsg.flags.rtr = ((line[0] == 'r') || (line[0] == 'R'));

    // upper case -> extended identifier
    if (line[0] < 'Z') {
        canmsg.flags.extended = 1;
        idlen = 8;
    } else {
        canmsg.flags.extended = 0;
        idlen = 3;
    }

    if (!parseHex(&line[1], idlen, &temp)) return 0;
    canmsg.id = temp;

    if (!parseHex(&line[1 + idlen], 1, &temp)) return 0;
    canmsg.dlc = temp;

    if (!canmsg.flags.rtr) {
        unsigned char i;
        unsigned char length = canmsg.dlc;
        if (length > 8) length = 8;
        for (i = 0; i < length; i++) {
            if (!parseHex(&line[idlen + 2 + i*2], 2, &temp)) return 0;
            canmsg.data[i] = temp;
        }
    }

    return mcp2515_send_message(&canmsg);
}

/**
 * Parse given command line
 *
 * @param line Line string to parse
 */
void parseLine(char * line) {

	unsigned char result = BELL;
	
	switch (line[0]) 
	{
		case 'S': // Setup with standard CAN bitrates
			if (state == STATE_CONFIG)
			{
				switch (line[1]) 
				{
					case '0': mcp2515_set_bittiming(MCP2515_TIMINGS_10K);  result = 13; break;
					case '1': mcp2515_set_bittiming(MCP2515_TIMINGS_20K);  result = 13; break;
					case '2': mcp2515_set_bittiming(MCP2515_TIMINGS_50K);  result = 13; break;
					case '3': mcp2515_set_bittiming(MCP2515_TIMINGS_100K); result = 13; break;
					case '4': mcp2515_set_bittiming(MCP2515_TIMINGS_125K); result = 13; break;
					case '5': mcp2515_set_bittiming(MCP2515_TIMINGS_250K); result = 13; break;
					case '6': mcp2515_set_bittiming(MCP2515_TIMINGS_500K); result = 13; break;
					case '7': mcp2515_set_bittiming(MCP2515_TIMINGS_800K); result = 13; break;
					case '8': mcp2515_set_bittiming(MCP2515_TIMINGS_1M);   result = 13; break;
				}

			}
			break;
		case 's': // Setup with user defined timing settings for CNF1/CNF2/CNF3
			if (state == STATE_CONFIG)
			{
				unsigned long cnf1, cnf2, cnf3;
				if (parseHex(&line[1], 2, &cnf1) && parseHex(&line[3], 2, &cnf2) && parseHex(&line[5], 2, &cnf3)) {
					mcp2515_set_bittiming(cnf1, cnf2, cnf3);
					result = 13;
				}
			} 
			break;
		case 'G': // Read given MCP2515 register
			if (!fuzz)
			{
				unsigned long address;
				if (parseHex(&line[1], 2, &address)) 
				{
					unsigned char value = mcp2515_read_register(address);
					sendByteHex(value);
					result = 13;
				}
			}
			break;
		case 'W': // Write given MCP2515 register
			if (!fuzz)
			{
				unsigned long address, data;
				if (parseHex(&line[1], 2, &address) && parseHex(&line[3], 2, &data)) {
					mcp2515_write_register(address, data);
					result = 13;
				}

			}
			break;
		case 'O': // Open CAN channel
			if (state == STATE_CONFIG)
			{
				mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x00); // set normal operating mode

				clock_reset();

				state = STATE_OPEN;
				result = 13;
			}
			break; 
		case 'l': // Loop-back mode
			if (state == STATE_CONFIG)
			{
				mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x40); // set loop-back

				state = STATE_OPEN;
				result = 13;
			}
			break; 
		case 'L': // Open CAN channel in listen-only mode
			if (state == STATE_CONFIG)
			{
				mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x60); // set listen-only mode

				state = STATE_LISTEN;
				result = 13;
			}
			break; 
		case 'C': // Close CAN channel
			if (state != STATE_CONFIG)
			{
				mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x80); // set configuration mode

				state = STATE_CONFIG;
				result = 13;
			}
			if (fuzz)
			{
				ResetFuzzer();
			}
			break; 
		case 'r': // Transmit standard RTR (11 bit) frame
		case 'R': // Transmit extended RTR (29 bit) frame
		case 't': // Transmit standard (11 bit) frame
		case 'T': // Transmit extended (29 bit) frame
			if (state == STATE_OPEN)
			{
				if (transmitStd(line)) {
					if (line[0] < 'Z') printf("%c",'Z');
					else printf("%c",'z');
					result = 13;
				}

			}
			break;        
		case 'F': // Read status flags
			if (!fuzz)
			{
				unsigned char flags = mcp2515_read_register(MCP2515_REG_EFLG);
				unsigned char status = 0;

				if (flags & 0x01) status |= 0x04; // error warning
				if (flags & 0xC0) status |= 0x08; // data overrun
				if (flags & 0x18) status |= 0x20; // passive error
				if (flags & 0x20) status |= 0x80; // bus error

				printf("%c",'F');
				sendByteHex(status);
				result = 13;
			}
			break;
		case 'f': // CAN Fuzzing
			if (state == STATE_OPEN)
			{
				if (line[1]=='z' && fuzz && !fuzzpause)  //pause fuzzing
				{
					pauseflag = 1;
					return ;
				}
				else if (line[1] == 'P' && fuzz)  //Stop Fuzzing
				{
					stopflag = 1;
					return ;
				}
				else if (line[1]=='r' && fuzz && fuzzpause)  //Resume Fuzzing
				{
					if (fuzz)
					{
						fuzzpause = 0;
						HAL_TIM_Base_Start(&htim5);
						result = 13;
						printf("f");
					}
					
				}
				else if ((line[1]=='d' || line[1]=='D')&&(!fuzz)) //config & start fuzzing
				{
					pauseflag = 0;
					stopflag = 0;
					if (config_fuzzer(&line[1]))
					{
						for (int i = 0; i < fuzzdlc; i++)
						{
							fuzzdata[i] = fuzzfrom[i];
						}
						fuzz = 1;						
						HAL_TIM_Base_Start(&htim5);
						result = 13;
						printf("f");
					}
					
				}
			}
			break;
		case 'Z': // Set time stamping
			if (!fuzz)
			{
				unsigned long stamping;
				if (parseHex(&line[1], 1, &stamping)) {
					timestamping = (stamping != 0);
					result = 13;
				}
			}
			break;
		case 'm': // Set accpetance filter mask
			if (state == STATE_CONFIG)
			{
				unsigned long am0, am1, am2, am3;
				if (parseHex(&line[1], 2, &am0) && parseHex(&line[3], 2, &am1) && parseHex(&line[5], 2, &am2) && parseHex(&line[7], 2, &am3)) {
					mcp2515_set_SJA1000_filter_mask(am0, am1, am2, am3);
					result = 13;
				}
			} 
			break;
		case 'M': // Set accpetance filter code
			if (state == STATE_CONFIG)
			{
				unsigned long ac0, ac1, ac2, ac3;
				if (parseHex(&line[1], 2, &ac0) && parseHex(&line[3], 2, &ac1) && parseHex(&line[5], 2, &ac2) && parseHex(&line[7], 2, &ac3)) {
					mcp2515_set_SJA1000_filter_code(ac0, ac1, ac2, ac3);
					result = 13;
				}
			} 
			break;
			case 'V': // Get hardware version
			if (!fuzz)
			{

				printf("%c",'V');
				sendByteHex(VERSION_HARDWARE_MAJOR);
				sendByteHex(VERSION_HARDWARE_MINOR);
				result = 13;
			}
			break;
		case 'v': // Get firmware version
			if (!fuzz)
			{
					
				printf("%c",'v');
				sendByteHex(VERSION_FIRMWARE_MAJOR);
				sendByteHex(VERSION_FIRMWARE_MINOR);
				result = 13;
			}
			break;
		case 'N': // Get serial number
			if (!fuzz)
			{
				printf("%c",'N');
				sendHex(0xFFFF, 4);
			/*
				if (usb_serialNumberAvailable()) {
						usb_putch(usb_string_serial[10]);
						usb_putch(usb_string_serial[12]);
						usb_putch(usb_string_serial[14]);
						usb_putch(usb_string_serial[16]);
				} else {
						sendHex(0xFFFF, 4);
				}*/
				result = 13;
			}
			break; 
		case 'b':  //RESET Device
			ResetFuzzer();
			k_state = K_UNKNOWN;
			j1850_mode = MODE_UNKNOWN;
			activatetask = 0;
			kmsgpending = 0;
			p3_timer = 0;
			sendkl = 0;
			Terminal_Resistor_Open_Circuit;
			if (state != STATE_CONFIG)
			{
				mcp2515_bit_modify(MCP2515_REG_CANCTRL, 0xE0, 0x80); // set configuration mode
				state = STATE_CONFIG;
			}
			
			result = 13;
			break;
		case 'k':  //K Line transreceiver;
			if (kmsgpending==0 && (!fuzz))
			{
				switch(line[1])
				{
					case 'a': //activate Bus
						switch (line[2])
						{
							case 'i': 
								if (k_state!= K_ISO9141_ACTIVE)
								{
									if (activatetask==0) 
									{
										activatetask = 1;
										return ;
									}
								}
								break; 
							case 'I': // 5 baud activate KWP2000
								if (k_state!= K_KWP2000_5BAUD_ACTIVE)
								{
									if (activatetask==0) 
									{
										activatetask = 2;
										return ;
									}
								}
								break; 
							case 'k': //Fast init KWP2000
								if (k_state != K_KWP2000_FAST_ACTIVE)
								{
									KWP2000_Fast_Init();
									k_state = K_KWP2000_FAST_ACTIVE;
									p3_timer = P3_TIMEOUT;
								}
								printf("%c",'o');
								result = 13;
								break;
						}
						break;
					case 'd': //deactivate Bus
						k_state = K_UNKNOWN;
						sendkl = 0;
						result = 13;
						break;
					case 'i':   //i stand for ISO9141-2
					case 'I': //I stand for KWP2000_5BAUD
					case 'k': //k stand for KWP2000_FAST
						if(k_state != K_UNKNOWN)
						{
							if (KWP2000FastMessage(&line[2]))
							{
								kmsgpending = 1;
								sendkl = 0;
								p3_timer = P3_TIMEOUT;
								printf("%c",'x');
								result = 13;
							}
						}
						break;
				}
			}
			break;
		case 'j':  //j1850 PWM/VPW transreceiver
			if (!fuzz) 
			{
				switch(line[1])
				{
					case 'p':  //PWM Mode
						if (j1850_mode != MODE_PWM)
						{
							j1850_pwm_init();
						}
						if (j1850_mode == MODE_PWM)
						{
							printf("x\r");
							if (J1850PWMMessage(&line[2]))
							{
								
								return ;
							}
						}
						break;
					case 'v':  //VPW mode
						if (j1850_mode != MODE_VPW)
						{
							j1850_vpw_init();
						}
						if (j1850_mode == MODE_VPW)
						{
							if (J1850VPWMessage(&line[2]))
							{
								printf("%c",'x');
								result = 13;
								
							}
						}
						break;
				}
			}
			break;
		case 'U':  //firmware upgrade
			__set_FAULTMASK(1);
			HAL_NVIC_SystemReset();
			break;
		case 'A': //terminal resistor open circuit;
			Terminal_Resistor_Open_Circuit;
			result = 13;
			break;
		case 'a': //terminal resistor 120 Ohms;
			Terminal_Resistor_120Ohms;
			result = 13;
			break;		
	}	
	
	printf("%c",result);
}

/**
 * Get next character of given can message in ascii format
 * 
 * @param canmsg Pointer to can message
 * @param step Current step
 * @return Next character to print out
 */
char canmsg2ascii_getNextChar(canmsg_t * canmsg, unsigned char * step) 
{
    
    char ch = BELL;
    char newstep = *step;       
    
    if (*step == RX_STEP_TYPE) {
        
            // type
             
            if (canmsg->flags.extended) {                 
                newstep = RX_STEP_ID_EXT;                
                if (canmsg->flags.rtr) ch = 'R';
                else ch = 'T';
            } else {
                newstep = RX_STEP_ID_STD;
                if (canmsg->flags.rtr) ch = 'r';
                else ch = 't';
            }        
             
    } else if (*step < RX_STEP_DLC) {

	// id        

        unsigned char i = *step - 1;
        unsigned char * id_bp = (unsigned char*) &canmsg->id;
        ch = id_bp[3 - (i / 2)];
        if ((i % 2) == 0) ch = ch >> 4;
        
        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';
        
        newstep++;
        
    } else if (*step < RX_STEP_DATA) {

	// length        

        ch = canmsg->dlc;
        
        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';

        if ((canmsg->dlc == 0) || canmsg->flags.rtr) newstep = RX_STEP_TIMESTAMP;
        else newstep++;
        
    } else if (*step < RX_STEP_TIMESTAMP) {
        
        // data        

        unsigned char i = *step - RX_STEP_DATA;
        ch = canmsg->data[i / 2];
        if ((i % 2) == 0) ch = ch >> 4;
        
        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';
        
        newstep++;        
        if (newstep - RX_STEP_DATA == canmsg->dlc*2) newstep = RX_STEP_TIMESTAMP;
        
    } else if (timestamping && (*step < RX_STEP_CR)) {
        
        // timestamp
        
        unsigned char i = *step - RX_STEP_TIMESTAMP;
        if (i < 2) ch = (canmsg->timestamp >> 8) & 0xff;
        else ch = canmsg->timestamp & 0xff;
        if ((i % 2) == 0) ch = ch >> 4;
        
        ch = ch & 0xF;
        if (ch > 9) ch = ch - 10 + 'A';
        else ch = ch + '0';
        
        newstep++;        
        
    } else {
        
        // linebreak
        
        ch = 13;
        newstep = RX_STEP_FINISHED;
    }
    
    *step = newstep;
    return ch;
}



void ResetFuzzer() 
{
	int i;
	fuzz = 0;
	fuzzid = 0;
	fuzzdlc = 0;
	fuzzprio = 0;
	fuzzpause = 0;
	fuzzfin = 0;
	fuzzperiod = 0;
	fuzzuntil = 0;
	fuzzuntilmask = 0x00;
	comparelength = 0;
	fuzzextend = 0;
	for (i = 0; i < 8; i++)
	{
		fuzzdata[i] = 0;
		fuzzfrom[i] = 0;
		fuzzto[i] = 0;
		fuzzuntildata[i] = 0;
	}
	HAL_TIM_Base_Stop(&htim5);
	__HAL_TIM_SET_COUNTER(&htim5,0);
}

void sendFuzzProcess()
{
	if (fuzzextend)
	{
		printf("%c",'G');
		printf("%08x",fuzzid);
	}
	else
	{
		printf("%c",'g');
		printf("%03x",fuzzid);
	}
	printf("%x",fuzzdlc);
	for (int i=0;i<fuzzdlc;i++)
	{
		printf("%02x",fuzzdata[i]);
	}
	
	printf("\r");
	
}

unsigned char config_fuzzer(char *config)
{
	unsigned long temp;
	int idlen;
	int pos,pos2;
	switch (config[0])
	{
		case 'd':
			fuzzextend = 0;
			idlen = 3;
			break;
		case 'D':
			fuzzextend = 1;
			idlen = 8;
			break;
		default:
			return 0;
	}
	if (!parseHex(&config[1], idlen, &temp)) return 0;
	fuzzid = temp;
	if (!parseHex(&config[1+idlen], 1, &temp)) return 0;
	fuzzdlc = temp;
	pos = idlen +2;
	int i;
	for (i = 0;i< fuzzdlc;i++)
	{
		if (!parseHex(&config[pos + 1 + i*2], 2, &temp)) return 0;
		fuzzfrom[i] = temp;
		if (!parseHex(&config[pos + 1 + fuzzdlc*2 + i*2], 2, &temp)) return 0;
		fuzzto[i] = temp;
	}
	
	if (config[pos + 1 + fuzzdlc*4]!='g'&&config[pos + 1 + fuzzdlc*4]!='G')
		return 0;
	if (config[pos + 1 + fuzzdlc*4]=='g')
		fuzzprio = 0;
	else
		fuzzprio = 1;
	pos ++;
	if (config[pos + 1 + fuzzdlc*4]!='p')
		return 0;
	for (i = pos + 1 + fuzzdlc*4+1;config[i]!='u'&&config[i]!='\0';i++);
	if (config[pos-1]=='s') 
	{
		fuzzuntil = 1;
		if (config[i]=='\0') return 0;
		int plen = i-(pos + 1 + fuzzdlc*4) - 1;
		if (!parseHex(&config[pos + 1 +fuzzdlc*4+1], plen, &temp)) return 0;
		fuzzperiod = temp;
		pos2 = i+1;
		for (i = 0;i< 8;i++)
		{
			if (!parseHex(&config[pos2 + i*2], 2, &temp)) return 0;
			fuzzuntildata[i] = temp;
		}
		pos2 += 16;
		if (config[pos2]!='m') return 0;
		if (!parseHex(&config[pos2 + 1], 2, &temp)) return 0;
		fuzzuntilmask = temp;
		
		
	}
	else if (config[pos-1]=='S' && config[i]=='\0' )
	{
		int plen = i-(pos + 1 + fuzzdlc*4) - 1;
		if (!parseHex(&config[pos + 1 +fuzzdlc*4+1], plen, &temp)) return 0;
		fuzzperiod = temp;
	}
	else 
	{
		return 0;
	}
	return 1;
}


u8 KWP2000FastMessage(char* data)
{
	unsigned long len,temp;
	int i;
	if (!parseHex(data,2,&len)) return 0;
	for (i = 0;i<len; i++) 
	{
		if (!parseHex(&data[2+i*2],2,&temp)) return 0;
		sendmessage[i] = temp;
	}
	
	sendmessage[len-1] = checksum(sendmessage,len-1);
	msglen = len;
	return 1;
}


u8 J1850VPWMessage(char* data)
{
	unsigned long len,temp;
	int i;
	if (!parseHex(data,2,&len)) return 0;
	for (i = 0;i<len; i++) 
	{
		if (!parseHex(&data[2+i*2],2,&temp)) return 0;
		j1850_send[i] = temp;
	}
	
	j1850_send[len-1] = j1850_crc((u8*)j1850_send,len-1);
	j1850_msglen = len;
	
	if (j1850_vpw_send_msg((u8*)j1850_send,j1850_msglen)==1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

u8 J1850PWMMessage(char* data)
{
	unsigned long len,temp;
	u8 recvbuffer[12];
	int i;
	if (!parseHex(data,2,&len)) return 0;
	for (i = 0;i<len; i++) 
	{
		if (!parseHex(&data[2+i*2],2,&temp)) return 0;
		j1850_send[i] = temp;
	}
	
	j1850_send[len-1] = j1850_crc((u8*)j1850_send,len-1);
	j1850_msglen = len;
	

	j1850_pwm_send_msg((u8*)j1850_send,j1850_msglen);
	
	u8 cnt = j1850_pwm_recv_msg((u8*)recvbuffer);
	if (cnt>0 && (!(cnt&0x80)))
	{
		if (1) 
		{
			printf("%c",'j');
			for (int i=0;i<cnt;i++)
			{
				printf("%02x",recvbuffer[i]);
			}
			printf("\r");
		} 
		cnt = 0;	
	}

	
	return 1;
}

