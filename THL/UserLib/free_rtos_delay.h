#ifndef __FREE_RTOS_DELAY_H
#define __FREE_RTOS_DELAY_H
#include "stm32f10x.h"

void TIM2_Delay_Init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif
