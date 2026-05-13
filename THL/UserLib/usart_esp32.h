#ifndef __USART_ESP32_H
#define __USART_ESP32_H

#include "stm32f10x.h"
#include <stdint.h>

// 定义通信包结构
typedef struct {
    float data1;      // 第一个浮点数据
    float data2;      // 第二个浮点数据
    uint16_t data3;   // 第三个无符号短整型数据
    uint16_t anomaly_flag; // 新增：1表示异常，0表示正常
} ESP32_Packet_t;

// 函数声明
void USART2_ESP32_Init(uint32_t baudrate);
void ESP32_SendPacket(const ESP32_Packet_t *packet);

#endif /* __USART_ESP32_H */
