#include "Serial.h"
#include <stdio.h>


void Serial_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

    GPIO_InitTypeDef GPIO_Struct = {0};
    GPIO_Struct.GPIO_Pin = GPIO_Pin_9;
    GPIO_Struct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_Struct);

    GPIO_Struct.GPIO_Pin = GPIO_Pin_10;
    GPIO_Struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_Struct);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
    
    USART_InitTypeDef USART_Struct = {0};
    USART_Struct.USART_BaudRate = 115200;
    USART_Struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Struct.USART_Parity = USART_Parity_No;
    USART_Struct.USART_StopBits = USART_StopBits_1;
    USART_Struct.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_Struct);

    USART_Cmd(USART1, ENABLE);



}

int fputc(int ch, FILE *f)
{
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, ch);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    return ch;
}

