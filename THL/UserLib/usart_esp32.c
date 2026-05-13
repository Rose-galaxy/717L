#include "usart_esp32.h"
#include <stdio.h>

/**
 * @brief 初始化 USART2 用于与 ESP32 通信
 * @param baudrate 波特率，通常使用 115200
 */
void USART2_ESP32_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 1. 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // GPIOA 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // USART2 时钟

    // 2. 配置 GPIO (PA2: TX, PA3: RX)
    // TX Pin (PA2)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // RX Pin (PA3)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. 配置 USART2
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // 收发模式
    USART_Init(USART2, &USART_InitStructure);

    // 4. 配置中断 (如果需要接收数据，可以启用 RXNE 中断，此处仅演示发送，故不强制启用中断)
    // 如果后续需要接收 ESP32 数据，请取消以下注释并配置 NVIC
    /*
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    */

    // 5. 使能 USART2
    USART_Cmd(USART2, ENABLE);
}

/**
 * @brief 通过 USART2 发送一个字节
 * @param data 要发送的字节
 */
static void USART2_SendByte(uint8_t data)
{
    // 等待发送寄存器为空
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    // 发送数据
    USART_SendData(USART2, data);
    // 等待发送完成
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}

/**
 * @brief 发送通信包给 ESP32
 * @param packet 指向 ESP32_Packet_t 结构体的指针
 * 
 * 注意：这里直接发送结构体的二进制数据。
 * 如果 ESP32 端使用 Python/C++ 解析，需确保字节序一致（STM32 通常为 Little-Endian）。
 * 为了增加可靠性，实际项目中建议添加帧头(如 0xAA 0x55)、长度字段和校验和。
 */
void ESP32_SendPacket(const ESP32_Packet_t *packet)
{
    if (packet == NULL) return;

    const uint8_t *data_ptr = (const uint8_t *)packet;
    uint16_t i;
    uint16_t packet_size = sizeof(ESP32_Packet_t);

    // 可选：发送帧头 (例如 0xAA 0x55)，方便 ESP32 识别包开始
    USART2_SendByte(0xAA);
    USART2_SendByte(0x55);

    // 发送结构体数据
    for (i = 0; i < packet_size; i++)
    {
        USART2_SendByte(data_ptr[i]);
    }

    //可选：发送帧尾 (例如 0x0D 0x0A)
    USART2_SendByte(0x0D);
    USART2_SendByte(0x0A);
}
