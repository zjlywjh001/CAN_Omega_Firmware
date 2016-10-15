//
//  led.h
//  
//
//
//

#ifndef _LED_H
#define _LED_H

#include "define.h"

#define LED_BLINK_TIME 50


#define TXLED_ON HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
#define TXLED_OFF HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
#define TXLED_TOG HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
#define RXLED_ON HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
#define RXLED_OFF HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
#define RXLED_TOG HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);

extern char txledstatus;
extern char rxledstatus;

extern int rxledcounter;
extern int txledcounter;

void flashTXLED(void);
void flashRXLED(void); 

#endif

