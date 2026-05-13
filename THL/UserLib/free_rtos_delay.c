#include "free_rtos_delay.h"
#include "FreeRTOS.h"
#include "task.h"

// 72MHz 主频,TIM2 分频到1MHz实现1us精度
void TIM2_Delay_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    // 需要使能APB1总线上的TIM2时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // 设置预分频器为72-1，使定时器时钟频率为72MHz/72=1MHz，即1us每计数
    TIM_TimeBaseStruct.TIM_Prescaler = 72 - 1;
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_Period = 0xFFFF;  // 计数上限
    TIM_TimeBaseStruct.TIM_ClockDivision = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);
    TIM_Cmd(TIM2, ENABLE);
}

void delay_us(uint32_t us)
{
    uint32_t start = TIM_GetCounter(TIM2);
    while((TIM_GetCounter(TIM2) - start) < us);
}

void delay_ms(uint32_t ms)
{
    if (ms < 100)  // 短延时用忙等
        delay_us(ms * 1000);
    else            // 长延时用RTOS调度
        vTaskDelay(pdMS_TO_TICKS(ms));
}
