#include "light_sensor.h"

//问题：数据取反，初始化存在错误
// 初始化 GPIO + ADC
void LIGHT_Sensor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    ADC_InitTypeDef  ADC_InitStruct;

    // 开时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);

    // // ------------------- PA0 上拉输入（DO）
    // GPIO_InitStruct.GPIO_Pin  = LIGHT_DO_GPIO_PIN;
    // GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    // GPIO_Init(LIGHT_DO_GPIO_PORT, &GPIO_InitStruct);

    // ------------------- PA6 模拟输入（AO）
    GPIO_InitStruct.GPIO_Pin  = LIGHT_AO_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;  // 模拟输入
    GPIO_Init(LIGHT_AO_GPIO_PORT, &GPIO_InitStruct);

    // ------------------- ADC1 配置
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // ADC 时钟 12M

    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStruct);

    ADC_Cmd(ADC1, ENABLE);

    // ADC 校准
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

// 读 DO 引脚（0=亮，1=暗）
uint8_t LIGHT_Read_DO(void)
{
    return GPIO_ReadInputDataBit(LIGHT_DO_GPIO_PORT, LIGHT_DO_GPIO_PIN);
}

// 读 AO 模拟值（0~4095）
uint16_t LIGHT_Read_AO(void)
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return (4096 - ADC_GetConversionValue(ADC1));
}
