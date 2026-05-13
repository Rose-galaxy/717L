#ifndef __DHT11_H
#define __DHT11_H
#include "stm32f10x.h"

// 可自行改引脚
#define DHT11_GPIO_PORT     GPIOB
#define DHT11_GPIO_PIN      GPIO_Pin_7

#define DHT11_PIN_LOW()     GPIO_ResetBits(DHT11_GPIO_PORT,DHT11_GPIO_PIN)
#define DHT11_PIN_HIGH()    GPIO_SetBits(DHT11_GPIO_PORT,DHT11_GPIO_PIN)

extern uint8_t DHT_Temp;
extern uint8_t DHT_Humi;

void DHT11_Init(void);
uint8_t DHT11_ReadData(void);

#endif
