
#include "define.h"
#include "led.h"

char txledstatus = 0;
char rxledstatus = 0;

int rxledcounter = 0;
int txledcounter = 0;

void flashTXLED() 
{
	if (txledstatus==0) 
	{
		txledstatus = 1;
		TXLED_ON;
	}

}

void flashRXLED() 
{
	if (rxledstatus==0) 
	{
		rxledstatus = 1;
		RXLED_ON;
	}

}

