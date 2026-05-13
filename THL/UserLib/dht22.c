#include "dht22.h"
#include "free_rtos_delay.h"
#include "FreeRTOS.h"
#include "task.h"

// 超时时间(微秒)
#define DHT22_TIMEOUT_US    1000    // 1ms超时

// 时序阈值(放宽容忍度以适应实际信号抖动)
#define DHT22_RESPONSE_MIN  40      // 传感器响应最小(us)70
#define DHT22_RESPONSE_MAX  120     // 传感器响应最大(us)100
#define DHT22_BIT0_MIN      10      // 数据位"0"最小(us)15
#define DHT22_BIT0_MAX      40      // 数据位"0"最大(us)35  
#define DHT22_BIT1_MIN      60      // 数据位"1"最小(us)55
#define DHT22_BIT1_MAX      90      // 数据位"1"最大(us)85

// 修改 WaitForPinLevel 函数，添加调试版本
static uint8_t WaitForPinLevel(uint8_t level, uint32_t timeout_us)
{
    uint32_t start = TIM_GetCounter(TIM2);
    
    while(DHT22_READ() != level) {
        if((TIM_GetCounter(TIM2) - start) >= timeout_us) {
            // 添加调试：看卡在等待什么电平
            return 0;
        }
    }
    return 1;
}

// 测量一个脉冲的宽度(微秒)
static uint32_t MeasurePulseWidth(uint8_t level)
{
    // 先等待引脚变为指定电平
    if(!WaitForPinLevel(level, DHT22_TIMEOUT_US)) {
        return 0;
    }
    
    // 测量电平持续时间
    uint32_t start = TIM_GetCounter(TIM2);
    
    // 等待电平变化(如果测量高电平，则等待变为低电平)
    while(DHT22_READ() == level) {
        if((TIM_GetCounter(TIM2) - start) >= DHT22_TIMEOUT_US) {
            return 0; // 超时
        }
    }
    
    return (TIM_GetCounter(TIM2) - start);
}

// 发送起始信号
static void SendStartSignal(void)
{
    DHT22_SET_OUT();
    
    // 拉低至少18ms(通常建议18-20ms)
    DHT22_OUT_LOW();
    delay_ms(18);
    
    // 拉高20-40us
    DHT22_OUT_HIGH();
    delay_us(30);
    
    // 切换为输入模式，准备读取
    DHT22_SET_IN();
}

// 等待传感器响应
static DHT22_Status WaitForResponse(void)
{
    uint32_t pulse_width;
    
    // 等待传感器拉低(响应开始)
    if(!WaitForPinLevel(0, DHT22_TIMEOUT_US)) {
        return DHT22_ERROR_NO_RESPONSE;
    }
    
    // 测量低电平持续时间(应为80us)
    pulse_width = MeasurePulseWidth(0);
    if(pulse_width < DHT22_RESPONSE_MIN || pulse_width > DHT22_RESPONSE_MAX) {
        return DHT22_ERROR_TIMEOUT;
    }
    
    // 测量高电平持续时间(应为80us)
    pulse_width = MeasurePulseWidth(1);
    if(pulse_width < DHT22_RESPONSE_MIN || pulse_width > DHT22_RESPONSE_MAX) {
        return DHT22_ERROR_TIMEOUT;
    }
    
    return DHT22_OK;
}

// 读取一个位(0或1)
static int ReadBit(void)
{
    uint32_t pulse_width;
    
    // 等待低电平(50us的前导)
    if(!WaitForPinLevel(0, DHT22_TIMEOUT_US)) {
        return -1;
    }
    
    // 测量高电平宽度来判断数据位
    pulse_width = MeasurePulseWidth(1);
    if(pulse_width == 0) {
        return -1;
    }
    
    // 根据高电平宽度判断"0"或"1"
    if(pulse_width >= DHT22_BIT0_MIN && pulse_width <= DHT22_BIT0_MAX) {
        return 0;
    } 
    else if(pulse_width >= DHT22_BIT1_MIN && pulse_width <= DHT22_BIT1_MAX) {
        return 1;
    }
    
    return -1; // 不在有效范围内
}

// 读取40位数据
static DHT22_Status ReadData(uint8_t* buffer)
{
    int bit;
    uint8_t byte_index = 0;
    uint8_t bit_index = 0;
    
    // 清空缓冲区
    for(int i = 0; i < 5; i++) {
        buffer[i] = 0;
    }
    
    // 读取40位数据
    for(int i = 0; i < 40; i++) {
        bit = ReadBit();
        if(bit < 0) {
            return DHT22_ERROR_TIMEOUT;
        }
        
        // 将bit存入缓冲区
        buffer[byte_index] <<= 1;
        buffer[byte_index] |= bit;
        
        bit_index++;
        if(bit_index == 8) {
            bit_index = 0;
            byte_index++;
        }
    }
    
    return DHT22_OK;
}

// 校验数据
static uint8_t VerifyChecksum(uint8_t* data)
{
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    return (sum == data[4]);
}

// 解析数据，转换为浮点数
static void ParseData(uint8_t* raw_data, DHT22_Data* result)
{
    uint16_t humidity_raw = (raw_data[0] << 8) | raw_data[1];
    uint16_t temperature_raw = (raw_data[2] << 8) | raw_data[3];
    
    // DHT22数据格式:
    // 湿度: 高8位+低8位, 实际湿度 = 原始值 / 10
    result->humidity = humidity_raw / 10.0f;
    
    // 温度: 最高位为符号位(1表示负温度)
    if(temperature_raw & 0x8000) {
        // 负温度
        temperature_raw &= 0x7FFF;
        result->temperature = -(temperature_raw / 10.0f);
    } else {
        // 正温度
        result->temperature = temperature_raw / 10.0f;
    }
    
    result->checksum = raw_data[4];
}

// 初始化DHT22
void DHT22_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // 使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 初始化PA0为推挽输出
    GPIO_InitStruct.GPIO_Pin = DHT22_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
    
    // 初始状态拉高
    DHT22_OUT_HIGH();
    
    // 确保定时器已初始化
    TIM2_Delay_Init();
}

// 启动测量(发送起始信号)
void DHT22_StartMeasurement(void)
{
    // 进入临界区，禁止任务调度
    taskENTER_CRITICAL();
    
    SendStartSignal();
    
    taskEXIT_CRITICAL();
}

// 读取DHT22数据(完整读取流程)
DHT22_Status DHT22_Read(DHT22_Data* data)
{
    uint8_t raw_data[5];
    DHT22_Status status;
    
    if(data == NULL) {
        return DHT22_ERROR_TIMEOUT;
    }
    
    // 清空数据
    data->temperature = 0;
    data->humidity = 0;
    data->checksum = 0;
    
    // 进入临界区 - 关键时序操作不能被中断
    taskENTER_CRITICAL();
    
    // 1. 发送起始信号
    SendStartSignal();
    
    // 2. 等待传感器响应
    status = WaitForResponse();
    if(status != DHT22_OK) {
        taskEXIT_CRITICAL();
        data->status = status;
        return status;
    }
    
    // 3. 读取40位数据
    status = ReadData(raw_data);
    if(status != DHT22_OK) {
        taskEXIT_CRITICAL();
        data->status = status;
        return status;
    }
    
    // 退出临界区(校验和计算不在临界区内，因为不涉及时序)
    taskEXIT_CRITICAL();
    
    // 4. 校验数据
    if(!VerifyChecksum(raw_data)) {
        data->status = DHT22_ERROR_CHECKSUM;
        return DHT22_ERROR_CHECKSUM;
    }
    
    // 5. 解析数据
    ParseData(raw_data, data);
    data->status = DHT22_OK;
    
    return DHT22_OK;
}
