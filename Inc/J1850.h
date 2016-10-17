
#ifndef __J1850_H__
#define __J1850_H__

#include "define.h"
#include "tim.h"
 
 
#define j1850_active() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET)
#define j1850_passive() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET)

#define j1850_pwm_passive() {HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET); HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);}
#define j1850_pwm_active() {HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET); HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);}


#define is_j1850_active() (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6)==SET)
#define is_j1850_pwm_active() (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5)==SET)

 
// define error return codes
#define J1850_RETURN_CODE_UNKNOWN    0
#define J1850_RETURN_CODE_OK         1
#define J1850_RETURN_CODE_BUS_BUSY   2
#define J1850_RETURN_CODE_BUS_ERROR  3
#define J1850_RETURN_CODE_DATA_ERROR 4
#define J1850_RETURN_CODE_NO_DATA    5
#define J1850_RETURN_CODE_DATA       6

// define J1850 VPW timing requirements in accordance with SAE J1850 standard
// all pulse width times in us
// transmitting pulse width
#define TX_SHORT	64		// Short pulse nominal time
#define TX_LONG		128		// Long pulse nominal time
#define TX_SOF		200		// Start Of Frame nominal time
#define TX_EOD		200		// End Of Data nominal time
#define TX_EOF		280		// End Of Frame nominal time
#define TX_BRK		300		// Break nominal time
#define TX_IFS		300		// Inter Frame Separation nominal time
#define PWM_TX_SOF 20  //32us actually
#define PWM_TX_EOF 72   //
#define PWM_TX_SHORT 4  //8us actually
#define PWM_TX_LONG 12   //16us actually
#define PWM_TX_BIT 20  //24us actually

// see SAE J1850 chapter 6.6.2.5 for preferred use of In Frame Respond/Normalization pulse
#define TX_IFR_SHORT_CRC	64	// short In Frame Respond, IFR contain CRC
#define TX_IFR_LONG_NOCRC 128	// long In Frame Respond, IFR contain no CRC

// receiving pulse width
#define RX_SHORT_MIN	34	// minimum short pulse time
#define RX_SHORT_MAX	96	// maximum short pulse time
#define RX_LONG_MIN		96	// minimum long pulse time
#define RX_LONG_MAX		163	// maximum long pulse time
#define RX_SOF_MIN		163	// minimum start of frame time
#define RX_SOF_MAX		239	// maximum start of frame time
#define RX_EOD_MIN		163	// minimum end of data time
#define RX_EOD_MAX		239	// maximum end of data time
#define RX_EOF_MIN		239	// minimum end of frame time, ends at minimum IFS
#define RX_BRK_MIN		239	// minimum break time
#define RX_IFS_MIN		280	// minimum inter frame separation time, ends at next SOF

// see chapter 6.6.2.5 for preferred use of In Frame Respond/Normalization pulse
#define RX_IFR_SHORT_MIN	34		// minimum short in frame respond pulse time
#define RX_IFR_SHORT_MAX	96		// maximum short in frame respond pulse time
#define RX_IFR_LONG_MIN		96		// minimum long in frame respond pulse time
#define RX_IFR_LONG_MAX		63		// maximum long in frame respond pulse time

#define MODE_UNKNOWN 0
#define MODE_VPW 1
#define MODE_PWM 2

extern int idletimeout;
extern u8 j1850_mode;
//extern u8 j1850_msg_pending;
extern u8 j1850_send[12];
extern u8 j1850_recv[12];
extern u8 j1850_msglen;


static inline void timer1_start(void)
{
	__HAL_TIM_SET_COUNTER(&htim3,0);
	HAL_TIM_Base_Start(&htim3);
}

static inline void timer1_stop(void)
{
	HAL_TIM_Base_Stop(&htim3);
}

static inline void timer1_set(uint16_t val)
{
	__HAL_TIM_SET_COUNTER(&htim3,val);
}

extern void j1850_vpw_init(void);
extern void j1850_pwm_init(void);
extern uint8_t j1850_vpw_recv_msg(uint8_t *msg_buf );
extern uint8_t j1850_vpw_send_msg(uint8_t *msg_buf, int8_t nbytes);
extern uint8_t j1850_crc(uint8_t *msg_buf, int8_t nbytes);
uint8_t j1850_pwm_send_msg(uint8_t *msg_buf, int8_t nbytes);


#endif // __J1850_H__
