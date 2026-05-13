#ifndef __LIGHT_SENSOR_H
#define __LIGHT_SENSOR_H

#include "stm32f10x.h"

// DO 接 PA0
#define LIGHT_DO_GPIO_PORT    GPIOA
#define LIGHT_DO_GPIO_PIN     GPIO_Pin_5
#define LIGHT_DO_GPIO_RCC     RCC_APB2Periph_GPIOA

// AO 接 PA1 (ADC1_IN1)
#define LIGHT_AO_GPIO_PORT    GPIOA
#define LIGHT_AO_GPIO_PIN     GPIO_Pin_6

void LIGHT_Sensor_Init(void);  // 初始化
uint8_t LIGHT_Read_DO(void);   // 读数字量
uint16_t LIGHT_Read_AO(void);  // 读模拟量

#endif
