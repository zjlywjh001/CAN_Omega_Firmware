
#include "define.h"
#include "j1850.h"


int idletimeout;
u8 j1850_mode = MODE_UNKNOWN;
//u8 j1850_msg_pending = 0;
u8 j1850_send[12];
u8 j1850_recv[12];
u8 j1850_msglen;
u8 j1850_ifr = 0;

/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Init J1850 bus VPW mode driver
** 
** Parameters: none
** 
** Returns: none
** 
**--------------------------------------------------------------------------- 
*/ 
void j1850_vpw_init(void)
{
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	j1850_mode = MODE_VPW;
	/*Configure GPIO pins : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;  //set VPWIN pin
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/*Configure GPIO pins : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;  //set VPWOUT pin
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/*Configure GPIO pins : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;  //set PWMOUT- pin
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);  //turn off PWMOUT-
	
	/*Configure GPIO pins : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;  //set PWMIN pin as input
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	j1850_passive();	// set VPW pin in passive state
	
}

/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Init J1850 bus PWM mode driver
** 
** Parameters: none
** 
** Returns: none
** 
**--------------------------------------------------------------------------- 
*/ 
void j1850_pwm_init(void)
{
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	j1850_mode = MODE_PWM;
	/*Configure GPIO pins : PB6 */
  /*Configure GPIO pins : PB6 */
	
  GPIO_InitStruct.Pin = GPIO_PIN_6;  //set VPWIN pin as input
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/*Configure GPIO pins : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;  //set PWMOUT+ pin as output
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	
	/*Configure GPIO pins : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;  //set PWMOUT- pin
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/*Configure GPIO pins : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;  //set PWMIN pin as input
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	j1850_pwm_passive(); // set PWM pin in passive state
	
}



/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Wait for J1850 bus idle
** 
** Parameters: none
** 
** Returns: 1 //timeout
						0 //ok
** 
**--------------------------------------------------------------------------- 
*/ 
static int j1850_wait_idle(void)
{
	
	idletimeout = 1000; //max 1 second;
	while (is_j1850_active() && !idletimeout);
	
	if (idletimeout==0)
	{
		return 1;
	}
		
	
	return 0;
	
}

/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Wait for J1850 bus PWM mode idle
** 
** Parameters: none
** 
** Returns: 1 //timeout
						0 //ok
** 
**--------------------------------------------------------------------------- 
*/ 
static int j1850_pwm_wait_idle(void)
{
	
	idletimeout = 1000; //max 1 second;
	while (is_j1850_pwm_active() && !idletimeout);
	
	if (idletimeout==0)
	{
		return 1;
	}
		
	
	return 0;
	
}

/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Receive J1850 frame (max 12 bytes)
** 
** Parameters: Pointer to frame buffer
** 
** Returns: Number of received bytes OR in case of error, error code with
**          bit 7 set as error indication
**
**--------------------------------------------------------------------------- 
*/ 
uint8_t j1850_vpw_recv_msg(uint8_t *msg_buf )
{
	uint8_t nbits;			// bit position counter within a byte
	uint8_t nbytes;		// number of received bytes
	uint8_t bit_state;// used to compare bit state, active or passive
	/*
		wait for responds
	*/

	timer1_start();	
	while(!is_j1850_active())	// run as long bus is passive (IDLE)
	{
		if(__HAL_TIM_GET_COUNTER(&htim3) >= 100)	// check for 100us
		{
			timer1_stop();
			return J1850_RETURN_CODE_NO_DATA | 0x80;	// error, no responds within 100us
		}
	}

	// wait for SOF
	timer1_start();	// restart timer1
	while(is_j1850_active())	// run as long bus is active (SOF is an active symbol)
	{
		if(__HAL_TIM_GET_COUNTER(&htim3) >=  RX_SOF_MAX) return J1850_RETURN_CODE_BUS_ERROR | 0x80;	// error on SOF timeout
	}
	
	timer1_stop();
	if(__HAL_TIM_GET_COUNTER(&htim3) < RX_SOF_MIN) return J1850_RETURN_CODE_BUS_ERROR | 0x80;	// error, symbole was not SOF
	
	bit_state = is_j1850_active();	// store actual bus state
	timer1_start();
	for(nbytes = 0; nbytes < 12; ++nbytes)
	{
		nbits = 8;
		do
		{
			*msg_buf <<= 1;
			while(is_j1850_active() == bit_state) // compare last with actual bus state, wait for change
			{
				if(__HAL_TIM_GET_COUNTER(&htim3) >= RX_EOD_MIN	)	// check for EOD symbol
				{
					timer1_stop();
					return nbytes;	// return number of received bytes
				}
			}
			bit_state = is_j1850_active();	// store actual bus state
			uint16_t tcnt1_buf = __HAL_TIM_GET_COUNTER(&htim3);
			timer1_start();
			if( tcnt1_buf < RX_SHORT_MIN) return J1850_RETURN_CODE_BUS_ERROR | 0x80;	// error, pulse was to short

			// check for short active pulse = "1" bit
			if( (tcnt1_buf < RX_SHORT_MAX) && !is_j1850_active() )
				*msg_buf |= 1;

			// check for long passive pulse = "1" bit
			if( (tcnt1_buf > RX_LONG_MIN) && (tcnt1_buf < RX_LONG_MAX) && is_j1850_active() )
				*msg_buf |= 1;

		} while(--nbits);// end 8 bit while loop
		
		++msg_buf;	// store next byte
		
	}	// end 12 byte for loop

	// return after a maximum of 12 bytes
	timer1_stop();	
	return nbytes;
}

/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Send J1850 frame (maximum 12 bytes)
** 
** Parameters: Pointer to frame buffer, frame length
** 
** Returns: 0 = error
**          1 = OK
** 
**--------------------------------------------------------------------------- 
*/ 
uint8_t j1850_vpw_send_msg(uint8_t *msg_buf, int8_t nbytes)
{
	
	if(nbytes > 12)	return J1850_RETURN_CODE_DATA_ERROR;	// error, message to long, see SAE J1850

	if (j1850_wait_idle())
	{
		return 0;	// wait for idle bus
	
	}
	timer1_start();	
	j1850_active();	// set bus active
	
	while(__HAL_TIM_GET_COUNTER(&htim3) < TX_SOF);	// transmit SOF symbol

	
  uint16_t delay;		// bit delay time
	
	for (int i=0;i<nbytes;i++)
	{
		for (int j=0;j<=7;j++)
		{
			if ((j+1)%2!=0)
			{
				j1850_passive();
				timer1_start();
				
				delay = (msg_buf[i]&(1<<(7-j))) ? TX_LONG : TX_SHORT;	// send correct pulse lenght
				delay_us(2);
				while (__HAL_TIM_GET_COUNTER(&htim3) <= delay)	// wait
				{
					if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6)==SET)	// check for bus error
					{
						timer1_stop();
						return J1850_RETURN_CODE_BUS_ERROR;	// error, bus collision!
					}
				}
			}
			else 
			{
				j1850_active();
				timer1_start();
				
				delay = (msg_buf[i]&(1<<(7-j))) ? TX_SHORT : TX_LONG;	// send correct pulse lenght
        while (__HAL_TIM_GET_COUNTER(&htim3) <= delay);	// wait
				
				// no error check needed, ACTIVE dominates
			}
		}
	}
  
	 
  j1850_passive();	// send EOF symbol
  timer1_start();
  while (__HAL_TIM_GET_COUNTER(&htim3) <= TX_EOF); // wait for EOF complete
	timer1_stop();
  return J1850_RETURN_CODE_OK;	// no error
}

/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Send J1850 PWM frame (maximum 12 bytes)
** 
** Parameters: Pointer to frame buffer, frame length
** 
** Returns: 0 = error
**          other - IFR
** 
**--------------------------------------------------------------------------- 
*/ 
uint8_t j1850_pwm_send_msg(uint8_t *msg_buf, int8_t nbytes)
{
	u8 val = 0;
	
	j1850_ifr = msg_buf[2];
	if(nbytes > 12)	return J1850_RETURN_CODE_DATA_ERROR;	// error, message to long, see SAE J1850

//	if (j1850_pwm_wait_idle())
//		return 0;	// wait for idle bus

	
	j1850_pwm_active();	// set bus active
	//timer1_start();	
	//while(__HAL_TIM_GET_COUNTER(&htim3) < PWM_TX_SOF);	// transmit SOF symbol
	delay_us(30);
	
	
	if (j1850_pwm_wait_idle())
		return 0;	// wait for idle bus
	j1850_pwm_passive();
	
	delay_us(18);
	
	//while(__HAL_TIM_GET_COUNTER(&htim3) < PWM_TX_TP4);	// transmit SOF symbol
	
	
	uint16_t delay;		// bit delay time
	
	for (int i=0;i<nbytes;i++)
	{
		for (int j=0;j<=7;j++)
		{
			j1850_pwm_active();
			timer1_start();
			
			delay = (msg_buf[i]&(1<<(7-j))) ? PWM_TX_SHORT : PWM_TX_LONG;	// send correct pulse lenght
			while (__HAL_TIM_GET_COUNTER(&htim3) <= delay) {}	// wait
		
			j1850_pwm_passive();
			timer1_start();
			delay = (msg_buf[i]&(1<<(7-j))) ? (PWM_TX_BIT-PWM_TX_SHORT) : (PWM_TX_BIT-PWM_TX_LONG);
			delay_us(1);
			while (__HAL_TIM_GET_COUNTER(&htim3) <= delay)	// wait
			{
				if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5)==SET)	// check for bus error
				{
					timer1_stop();
					return J1850_RETURN_CODE_BUS_ERROR;	// error, bus collision!
				}
			}
		}
	}
	//j1850_pwm_active(); //end of last symbol
	
	//delay_us(5);
  j1850_passive();	// send EOF symbol
	
  timer1_start();

	
	
	//Wait for IFR
	while (__HAL_TIM_GET_COUNTER(&htim3) <= PWM_RX_TP4_MAX&&(!is_j1850_pwm_active())) ;
	
	if (__HAL_TIM_GET_COUNTER(&htim3) <= PWM_RX_TP4_MAX)
	{
		//Receive IFR
		
		for (int i=0;i<8;i++)
		{
			
			val <<= 1;
			timer1_start();
			
			while (__HAL_TIM_GET_COUNTER(&htim3) <= 9);
			
			if (!is_j1850_pwm_active())
			{
				val|=1;
			}
			else 
			{
				while (__HAL_TIM_GET_COUNTER(&htim3) <= PWM_RX_TP4_MAX&&(is_j1850_pwm_active())) ;
			}
			
			while (__HAL_TIM_GET_COUNTER(&htim3) <= PWM_RX_TP4_MAX&&(!is_j1850_pwm_active())) ;

		}

	}
	
	
  //while (__HAL_TIM_GET_COUNTER(&htim3) <= PWM_TX_EOF); // wait for EOF complete

	timer1_stop();

  return val;
}


/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Receive J1850 PWM frame (maximum 12 bytes)
** 
** Parameters: Pointer to frame buffer, frame length
** 
** Returns: 0 = error
**          other = OK
** 
**--------------------------------------------------------------------------- 
*/ 
uint8_t j1850_pwm_recv_msg(uint8_t *msg_buf)
{
	
	u8 val,nbytes;
	u16 delay;
	nbytes = 0;
	
	timer1_start();
	while (__HAL_TIM_GET_COUNTER(&htim3) <= 2000 && !is_j1850_pwm_active()); //wait for sof
	if (__HAL_TIM_GET_COUNTER(&htim3) > 2000)
	{
		timer1_stop();
		return J1850_RETURN_CODE_NO_DATA | 0x80;	// error, no responds
	}
	
	timer1_start();
	while (__HAL_TIM_GET_COUNTER(&htim3) <= 33 && is_j1850_pwm_active()); //wait for sof
	if (__HAL_TIM_GET_COUNTER(&htim3) < 25)  //not a sof
	{
		timer1_stop();
		return J1850_RETURN_CODE_BUS_ERROR | 0x80;	//  sof error
	}
	while (__HAL_TIM_GET_COUNTER(&htim3) <= 63 && !is_j1850_pwm_active()); //wait for sofwhile (!is_j1850_pwm_active());
	if (__HAL_TIM_GET_COUNTER(&htim3) > 51 )  //not a sof
	{
		timer1_stop();
		return J1850_RETURN_CODE_BUS_ERROR | 0x80;	//  sof error
	}
	
	while (1)
	{
		val = 0;
		
		for (int i=0;i<8;i++)
		{
			
			val <<= 1;
			timer1_start();
			
			while (__HAL_TIM_GET_COUNTER(&htim3) <= 8);
			
			if (!is_j1850_pwm_active())
			{
				val|=1;
			}
			else 
			{
				while (__HAL_TIM_GET_COUNTER(&htim3) < 48&&(is_j1850_pwm_active())) ;
			}
			
			while (__HAL_TIM_GET_COUNTER(&htim3) < 48&&(!is_j1850_pwm_active())) ;

		}
		if (__HAL_TIM_GET_COUNTER(&htim3) < 48)
		{
			msg_buf[nbytes++] = val;
		}
		else
		{
			break;
		}
	}
	
	// send IFR 
	for (int j=0;j<=7;j++)
	{
		j1850_pwm_active();
		timer1_start();
		
		delay = (j1850_ifr&(1<<(7-j))) ? PWM_TX_SHORT : PWM_TX_LONG;	// send correct pulse lenght
		while (__HAL_TIM_GET_COUNTER(&htim3) <= delay) {}	// wait
	
		j1850_pwm_passive();
		timer1_start();
		delay = (j1850_ifr&(1<<(7-j))) ? (PWM_TX_BIT-PWM_TX_SHORT) : (PWM_TX_BIT-PWM_TX_LONG);
		delay_us(1);
		while (__HAL_TIM_GET_COUNTER(&htim3) <= delay)	// wait
		{
			if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5)==SET)	// check for bus error
			{
				timer1_stop();
				return J1850_RETURN_CODE_BUS_ERROR | 0x80;	// error, bus collision!
			}
		}
	}
	
	timer1_stop();
		
	if (nbytes>12)
		nbytes = 12;
	
	return nbytes;
	
}


/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Calculate J1850 CRC	
** 
** Parameters: Pointer to frame buffer, frame length
** 
** Returns: CRC of frame
** 
**--------------------------------------------------------------------------- 
*/ 
// calculate J1850 message CRC
uint8_t j1850_crc(uint8_t *msg_buf, int8_t nbytes)
{
	uint8_t crc_reg=0xff,poly,byte_count,bit_count;
	uint8_t *byte_point;
	uint8_t bit_point;

	for (byte_count=0, byte_point=msg_buf; byte_count<nbytes; ++byte_count, ++byte_point)
	{
		for (bit_count=0, bit_point=0x80 ; bit_count<8; ++bit_count, bit_point>>=1)
		{
			if (bit_point & *byte_point)	// case for new bit = 1
			{
				if (crc_reg & 0x80)
					poly=1;	// define the polynomial
				else
					poly=0x1c;
				crc_reg= ( (crc_reg << 1) | 1) ^ poly;
			}
			else		// case for new bit = 0
			{
				poly=0;
				if (crc_reg & 0x80)
					poly=0x1d;
				crc_reg= (crc_reg << 1) ^ poly;
			}
		}
	}
	return ~crc_reg;	// Return CRC
}



