#ifndef __DHT22_H
#define __DHT22_H

#include "stm32f10x.h"  // 根据你的STM32型号调整

// DHT22引脚配置 - PA0
#define DHT22_PORT        GPIOA
#define DHT22_PIN         GPIO_Pin_0
#define DHT22_PIN_SOURCE  GPIO_PinSource0

// 引脚操作宏
#define DHT22_SET_OUT()   { GPIO_InitTypeDef GPIO_InitStruct; \
                            GPIO_InitStruct.GPIO_Pin = DHT22_PIN; \
                            GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP; \
                            GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; \
                            GPIO_Init(DHT22_PORT, &GPIO_InitStruct); }

#define DHT22_SET_IN()    { GPIO_InitTypeDef GPIO_InitStruct; \
                            GPIO_InitStruct.GPIO_Pin = DHT22_PIN; \
                            GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU; \
                            GPIO_Init(DHT22_PORT, &GPIO_InitStruct); }

#define DHT22_OUT_HIGH()  GPIO_SetBits(DHT22_PORT, DHT22_PIN)
#define DHT22_OUT_LOW()   GPIO_ResetBits(DHT22_PORT, DHT22_PIN)
#define DHT22_READ()      GPIO_ReadInputDataBit(DHT22_PORT, DHT22_PIN)

// 返回状态
typedef enum {
    DHT22_OK = 0,
    DHT22_ERROR_TIMEOUT,
    DHT22_ERROR_CHECKSUM,
    DHT22_ERROR_NO_RESPONSE
} DHT22_Status;

// 数据结构
typedef struct {
    float temperature;  // 温度(摄氏度)
    float humidity;     // 湿度(%)
    uint8_t checksum;   // 校验和
    DHT22_Status status; // 最后一次读取状态
} DHT22_Data;

// 函数声明
void DHT22_Init(void);
DHT22_Status DHT22_Read(DHT22_Data* data);
void DHT22_StartMeasurement(void);  // 触发一次测量(需等待2秒后读取)

#endif
