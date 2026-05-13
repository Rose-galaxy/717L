#include "dht11.h"
#include "free_rtos_delay.h"
#include <stdio.h>

uint8_t DHT_Temp = 0;
uint8_t DHT_Humi = 0;

// 配置为推挽输出
static void DHT11_Set_Out(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

// 配置为上拉输入
static void DHT11_Set_In(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    DHT11_Set_Out();
    DHT11_PIN_HIGH();
}

// 读取1位数据（带超时保护）
static uint8_t DHT11_Read_Bit(void)
{
    uint8_t dat = 0;
    uint16_t timeout = 0;
    
    // 等待低电平结束（带超时）
    timeout = 0;
    while(!GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN))
    {
        if(++timeout > 1000) return 0xFF;
    }
    
    delay_us(40);
    
    if(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN))
    {
        dat = 1;
    }
    
    // 等待高电平结束（带超时）
    timeout = 0;
    while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN))
    {
        if(++timeout > 1000) break;
    }
    
    return dat;
}

// 读取1字节
static uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat = 0;
    uint8_t bit;
    
    for(i = 0; i < 8; i++)
    {
        bit = DHT11_Read_Bit();
        if(bit == 0xFF) return 0xFF;
        
        dat <<= 1;
        dat |= bit;
    }
    return dat;
}

// FreeRTOS 下读取温湿度 成功返回1
uint8_t DHT11_ReadData(void)
{
    uint8_t buf[5], i;
    uint32_t timeout = 0;
    uint8_t retry = 0;
    
    // 最多重试5次
    for(retry = 0; retry < 5; retry++)
    {
        DHT11_Set_Out();
        DHT11_PIN_LOW();
        delay_ms(20);  // 拉低至少18ms
        DHT11_PIN_HIGH();
        delay_us(40);  // 拉高20-40us

        DHT11_Set_In();
        
        // 短暂延时让引脚稳定
        delay_us(20);

        // 等待DHT11应答（带超时保护）
        // DHT11应该在80us内拉低
        timeout = 0;
        while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN))
        {
            if(++timeout > 5000) break;  // 增加到5000，约500us
        }
        
        if(timeout >= 5000) {
            // 第一次超时，可能是时序问题，继续重试
            continue;
        }
        
        // 检测到低电平响应（应该持续80us）
        timeout = 0;
        while(!GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN))
        {
            if(++timeout > 5000) break;
        }
        
        if(timeout >= 5000) continue;
        
        // 检测到高电平（应该持续80us）
        timeout = 0;
        while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN))
        {
            if(++timeout > 5000) break;
        }
        
        if(timeout >= 5000) continue;

        // 开始读取40位数据
        for(i = 0; i < 5; i++)
        {
            buf[i] = DHT11_Read_Byte();
            if(buf[i] == 0xFF) break;
        }
        
        if(i < 5) continue;
        
        // 校验和判断
        if(buf[0] + buf[1] + buf[2] + buf[3] == buf[4])
        {
            DHT_Humi = buf[0];
            DHT_Temp = buf[2];
            return 1;
        }
        
        // 校验失败，继续重试
        delay_ms(10);
    }
    
    // 5次重试都失败
    return 0;
}
