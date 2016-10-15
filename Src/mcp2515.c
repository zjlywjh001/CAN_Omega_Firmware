
#include "define.h"
#include "mcp2515.h"
#include "tim.h"
/** current transmit buffer priority */
unsigned char txprio = 3;


/**
 * \brief Write to given register
 *
 * \param address Register address
 * \param data Value to write to given register
 */
void mcp2515_write_register(unsigned char address, unsigned char data) {

	// pull SS to low level
	MCP2515_SS0;
 
	SPI1_TransferByte(MCP2515_CMD_WRITE);
	SPI1_TransferByte(address);
	SPI1_TransferByte(data);
 
	// release SS
	MCP2515_SS1;
}

/**
 * \brief Read from given register
 *
 * \param address Register address
 * \return register value
 */
unsigned char mcp2515_read_register(unsigned char address) {

	unsigned char data;
 
	// pull SS to low level
	MCP2515_SS0;
 
	SPI1_TransferByte(MCP2515_CMD_READ);
	SPI1_TransferByte(address);   
	data = SPI1_TransferByte(0xff); 
 
	// release SS
	MCP2515_SS1;
 
	return data;
}

/**
 * \brief Modify bit of given register
 *
 * \param address Register address
 * \param mask Mask of bits to set
 * \param data Values to set
 *
 * This function works only on a few registers. Please check the datasheet!
 */
void mcp2515_bit_modify(unsigned char address, unsigned char mask, unsigned char data) {

	// pull SS to low level
	MCP2515_SS0;

	SPI1_TransferByte(MCP2515_CMD_BIT_MODIFY);
	SPI1_TransferByte(address);
	SPI1_TransferByte(mask);
	SPI1_TransferByte(data);

	// release SS
	MCP2515_SS1;
}

/**
 * \brief Initialize spi interface, reset the MCP2515 and activate clock output signal
 */
void mcp2515_init() {
	

	unsigned char dummy;

	while (++dummy) {};

	// reset device
	MCP2515_SS0; // SS
	SPI1_TransferByte(0xC0); // reset device
	MCP2515_SS1; // SS

	while (++dummy) {};


	mcp2515_write_register(MCP2515_REG_CANCTRL, 0x87); // set config mode, clock prescaling 1:8 and clock output

	// configure filter
	mcp2515_write_register(MCP2515_REG_RXB0CTRL, 0x00); // use filter for standard and extended frames
	mcp2515_write_register(MCP2515_REG_RXB1CTRL, 0x00); // use filter for standard and extended frames

	// initialize filter mask
	mcp2515_write_register(MCP2515_REG_RXM0SIDH, 0x00);
	mcp2515_write_register(MCP2515_REG_RXM0SIDL, 0x00);
	mcp2515_write_register(MCP2515_REG_RXM0EID8, 0x00);
	mcp2515_write_register(MCP2515_REG_RXM0EID0, 0x00);
	mcp2515_write_register(MCP2515_REG_RXM1SIDH, 0x00);
	mcp2515_write_register(MCP2515_REG_RXM1SIDL, 0x00);
	mcp2515_write_register(MCP2515_REG_RXM1EID8, 0x00);
	mcp2515_write_register(MCP2515_REG_RXM1EID0, 0x00);

	mcp2515_write_register(MCP2515_REG_CANINTE, 0x03); // RX interrupt
		
	mcp2515_set_bittiming(MCP2515_TIMINGS_500K); //default protocol: CAN500Kbps 11bit
	
}

/**
 * \brief Set filter mask of given SJA1000 register values
 *
 * \param amr0 Acceptence mask register 0
 * \param amr1 Acceptence mask register 1
 * \param amr2 Acceptence mask register 2
 * \param amr3 Acceptence mask register 3
 *
 * This function has only affect if mcp2515 is in configuration mode.
 * The filter mask is only set for the first 11 bit because of compatibility
 * issues between SJA1000 and MCP2515.
 */
void mcp2515_set_SJA1000_filter_mask(unsigned char amr0, unsigned char amr1, unsigned char amr2, unsigned char amr3) {

	// SJA1000 mask bit definition: 1 = accept without matching, 0 = do matching with acceptance code
	// MCP2515 mask bit definition: 0 = accept without matching, 1 = do matching with acceptance filter
	// -> invert mask

	// mask for filter 1
	mcp2515_write_register(MCP2515_REG_RXM0SIDH, ~amr0);
	mcp2515_write_register(MCP2515_REG_RXM0SIDL, (~amr1) & 0xE0);
	mcp2515_write_register(MCP2515_REG_RXM0EID8, 0x00);
	mcp2515_write_register(MCP2515_REG_RXM0EID0, 0x00);

	// mask for filter 2
	mcp2515_write_register(MCP2515_REG_RXM1SIDH, ~amr2);
	mcp2515_write_register(MCP2515_REG_RXM1SIDL, (~amr3) & 0xE0);
	mcp2515_write_register(MCP2515_REG_RXM1EID8, 0x00);
	mcp2515_write_register(MCP2515_REG_RXM1EID0, 0x00);

}

/**
 * \brief Set filter code of given SJA1000 register values
 *
 * \param amr0 Acceptence code register 0
 * \param amr1 Acceptence code register 1
 * \param amr2 Acceptence code register 2
 * \param amr3 Acceptence code register 3
 *
 * This function has only affect if mcp2515 is in configuration mode.
 */
void mcp2515_set_SJA1000_filter_code(unsigned char acr0, unsigned char acr1, unsigned char acr2, unsigned char acr3) {

	// acceptance code for filter 1
	mcp2515_write_register(MCP2515_REG_RXF0SIDH, acr0);
	mcp2515_write_register(MCP2515_REG_RXF0SIDL, (acr1) & 0xE0); // standard
	mcp2515_write_register(MCP2515_REG_RXF1SIDH, acr0);
	mcp2515_write_register(MCP2515_REG_RXF1SIDL, ((acr1) & 0xE0) | 0x08); // extended

	// acceptance code for filter 2
	mcp2515_write_register(MCP2515_REG_RXF2SIDH, acr2);
	mcp2515_write_register(MCP2515_REG_RXF2SIDL, (acr3) & 0xE0); // standard
	mcp2515_write_register(MCP2515_REG_RXF3SIDH, acr2);
	mcp2515_write_register(MCP2515_REG_RXF3SIDL, ((acr3) & 0xE0) | 0x08); // extended

	// fill remaining filters with zero
	mcp2515_write_register(MCP2515_REG_RXF4SIDH, 0x00);
	mcp2515_write_register(MCP2515_REG_RXF4SIDL, 0x00);
	mcp2515_write_register(MCP2515_REG_RXF5SIDH, 0x00);
	mcp2515_write_register(MCP2515_REG_RXF5SIDL, 0x00);
}


/**
 * \brief Set bit timing registers
 *
 * \param cnf1 Configuration register 1
 * \param cnf2 Configuration register 2
 * \param cnf3 Configuration register 3
 *
 * This function has only affect if mcp2515 is in configuration mode
 */
void mcp2515_set_bittiming(unsigned char cnf1, unsigned char cnf2, unsigned char cnf3) {

	mcp2515_write_register(MCP2515_REG_CNF1, cnf1);
	mcp2515_write_register(MCP2515_REG_CNF2, cnf2);
	mcp2515_write_register(MCP2515_REG_CNF3, cnf3);
}

/**
 * \brief Read status byte of MCP2515
 *
 * \return status byte of MCP2515
 */
unsigned char mcp2515_read_status() {

	// pull SS to low level
	MCP2515_SS0;
 
	SPI1_TransferByte(MCP2515_CMD_READ_STATUS);
	unsigned char status = SPI1_TransferByte(0xff);
 
	// release SS
	MCP2515_SS1;

	return status;
}


/**
 * \brief Read RX status byte of MCP2515
 *
 * \return RX status byte of MCP2515
 */
unsigned char mcp2515_rx_status() {

	// pull SS to low level
	MCP2515_SS0;
 
	SPI1_TransferByte(MCP2515_CMD_RX_STATUS);
	unsigned char status = SPI1_TransferByte(0xff);
 
	// release SS
	MCP2515_SS1;

	return status;
}

/**
 * \brief Send given CAN message
 *
 * \ p_canmsg Pointer to can message to send
 * \return 1 if transmitted successfully to MCP2515 transmit buffer, 0 on error (= no free buffer available)
 */
unsigned char mcp2515_send_message(canmsg_t * p_canmsg) {

	unsigned char status = mcp2515_read_status();
	unsigned char address;
	unsigned char ctrlreg;
	unsigned char length;

	// check length
	length = p_canmsg->dlc;
	if (length > 8) length = 8;

	// do some priority fiddling to get fifo behavior
	switch (status & 0x54) {
			
			case 0x00:
					// all three buffers free
					ctrlreg = MCP2515_REG_TXB2CTR;
					address = 0x04;            
					txprio = 3;
					break;
					
			case 0x40:
			case 0x44:
					ctrlreg = MCP2515_REG_TXB1CTR;
					address = 0x02;            
					break;
					
			case 0x10:
			case 0x50:
					ctrlreg = MCP2515_REG_TXB0CTR;
					address = 0x00;
					break;
					
			case 0x04:
			case 0x14:         
					ctrlreg = MCP2515_REG_TXB2CTR;
					address = 0x04;
					
					if (txprio == 0) {
							// set priority of buffer 1 and buffer 0 to highest
							mcp2515_bit_modify(MCP2515_REG_TXB1CTR, 0x03, 0x03);
							mcp2515_bit_modify(MCP2515_REG_TXB0CTR, 0x03, 0x03);
							txprio = 2;
					} else {
							txprio--;
					}            
					break;
					
			default:
					// no free transmit buffer
					return 0;           
	}


	// pull SS to low level
	MCP2515_SS0;

	SPI1_TransferByte(MCP2515_CMD_LOAD_TX | address);

	if (p_canmsg->flags.extended) {
			SPI1_TransferByte(p_canmsg->id >> 21);
			SPI1_TransferByte(((p_canmsg->id >> 13) & 0xe0) | ((p_canmsg->id >> 16) & 0x03) | 0x08);
			SPI1_TransferByte(p_canmsg->id >> 8);
			SPI1_TransferByte(p_canmsg->id);
	} else {
			SPI1_TransferByte(p_canmsg->id >> 3);
			SPI1_TransferByte(p_canmsg->id << 5);
			SPI1_TransferByte(0);
			SPI1_TransferByte(0);
	}

	// length and data
	if (p_canmsg->flags.rtr) {
			SPI1_TransferByte(p_canmsg->dlc | 0x40);
	} else {
			SPI1_TransferByte(p_canmsg->dlc);
			unsigned char i;
			for (i = 0; i < length; i++) {
					SPI1_TransferByte(p_canmsg->data[i]);
			}
	}

	// release SS
	MCP2515_SS1;

	delay_us(10);

	// request message to be transmitted
	mcp2515_write_register(ctrlreg, txprio | 0x08);
			
	return 1;
}

/*
 * \brief Read out one can message from MCP2515
 *
 * \param p_canmsg Pointer to can message structure to fill
 * \return 1 on success, 0 if there is no message to read
 */
unsigned char mcp2515_receive_message(canmsg_t * p_canmsg) {

	unsigned char status = mcp2515_rx_status();
	unsigned char address;    
	
	if (status & 0x40) 
	{
		address = 0x00;
	}
	else if (status & 0x80) 
	{
		address = 0x04;
	} 
	else 
	{
		// no message in receive buffer
		return 0;
	}

	// store timestamp
	p_canmsg->timestamp = clock_getMS();
	
	// store flags
	p_canmsg->flags.rtr = (status >> 3) & 0x01;
	p_canmsg->flags.extended = (status >> 4) & 0x01;

	// pull SS to low level
	MCP2515_SS0;
 
	SPI1_TransferByte(MCP2515_CMD_READ_RX | address);

	if (p_canmsg->flags.extended) 
	{
		p_canmsg->id =  (unsigned long) SPI1_TransferByte(0xff) << 21;
		unsigned long temp = SPI1_TransferByte(0xff);
		p_canmsg->id |= (temp & 0xe0) << 13;
		p_canmsg->id |= (temp & 0x03) << 16;
		p_canmsg->id |= (unsigned long) SPI1_TransferByte(0xff) << 8;
		p_canmsg->id |= (unsigned long) SPI1_TransferByte(0xff);
	} 
	else 
	{
		p_canmsg->id =  (unsigned long) SPI1_TransferByte(0xff) << 3;
		p_canmsg->id |= (unsigned long) SPI1_TransferByte(0xff) >> 5;
		SPI1_TransferByte(0xff);
		SPI1_TransferByte(0xff);
	}

	// get length and data
	p_canmsg->dlc = SPI1_TransferByte(0xff) & 0x0f;
	if (!p_canmsg->flags.rtr) 
	{
		unsigned char i;
		unsigned char length = p_canmsg->dlc;
		if (length > 8) length = 8;
		for (i = 0; i < length; i++) 
		{
			p_canmsg->data[i] = SPI1_TransferByte(0xff);
		}
	}

	// release SS
	MCP2515_SS1;

	if (address == 0) address = 1;
	else address = 2;
	mcp2515_bit_modify(MCP2515_REG_CANINTF, address, 0);

	return 1;
}

