#include "stm32f10x.h"
#include "freertos.h"
#include "task.h"
#include "OLED.h"
#include "Serial.h"
#include <stdio.h>
#include "free_rtos_delay.h"
#include "dht22.h"
#include "light_sensor.h"
#include "queue.h"
#include "usart_esp32.h"
#include "anomaly_detector.h"

// 滑动窗口配置
#define WINDOW_SIZE 20
#define ANOMALY_K   3.0f
 
static TaskHandle_t myTaskHandler = NULL;
static TaskHandle_t displayTaskHandler = NULL;
static TaskHandle_t usart2TaskHandler = NULL;
static TaskHandle_t light_sensorTaskHandler = NULL;
static TaskHandle_t dht22TaskHandler = NULL;

QueueHandle_t xDHT22Queue;
QueueHandle_t xLightQueue;


/// @brief 测试任务
/// @param arg 
static void myTask(void *arg );

/// @brief oled显示任务
/// @param arg 
static void displayTask(void *arg);

/// @brief 串口显示任务
/// @param arg
static void usartTask(void *arg);

/// @brief 光敏传感器任务
static void light_sensorTask(void *arg);

/// @brief 温湿度传感器任务
static void dht22Task(void *arg);

/// @brief 串口esp32任务
/// @param arg 
static void usart_esp32Task(void *arg);

static void printhello(void *arg);


int main(void)
{
    xDHT22Queue = xQueueCreate(5, sizeof(DHT22_Data *));
    xLightQueue = xQueueCreate(5, sizeof(uint16_t));
    GPIO_InitTypeDef GPIO_Initstruct;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    GPIO_Initstruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Initstruct.GPIO_Pin = GPIO_Pin_13;
    GPIO_Initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_Initstruct);    
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
    
    Serial_Init();
    OLED_Init();
    TIM2_Delay_Init();


    // 创建任务
// ... existing code ...
    // 创建任务 - 优化栈大小分配
    xTaskCreate(myTask, "myTask", 128, NULL, 1, &myTaskHandler);           // LED任务：128 words = 512 bytes
    xTaskCreate(dht22Task, "vDHT22_Task", 256, NULL, 3, &dht22TaskHandler); // DHT22任务：256 words = 1024 bytes
    xTaskCreate(displayTask, "displayTask", 384, NULL, 1, &displayTaskHandler); // 显示任务：384 words = 1536 bytes
    xTaskCreate(printhello, "usartTask", 128, NULL, 1, NULL);
    xTaskCreate(light_sensorTask, "sensorTask", 128, NULL, 2, &light_sensorTaskHandler); // 光敏任务：128 words = 512 bytes
    xTaskCreate(usart_esp32Task, "usart_esp32Task", 512, NULL, 2, &usart2TaskHandler);
// ... existing code ...
    vTaskStartScheduler();
 
    while (1)
    {
    }
}

static void myTask(void *arg )
{
    while(1)
    {
        GPIO_ResetBits(GPIOC,GPIO_Pin_13);      
        vTaskDelay(500);
        GPIO_SetBits(GPIOC,GPIO_Pin_13);
        vTaskDelay(500);
    }        
}

//统一的显示任务
static void displayTask(void *arg)
{
    while(1)
    {
        


        DHT22_Data *dht_data;
        uint32_t humi_val;
        int32_t temp_val;
        uint16_t light_val;
        uint32_t lasthumi_val = 0xFFFFFFFF;  // ✅ 初始化为不可能出现的值
        int32_t lasttemp_val = 0x7FFFFFFF;   // ✅ 确保首次一定更新
        uint16_t lastlight_val = 0xFFFF;     // ✅

        
        // 第2行：湿度

        if(xQueueReceive(xDHT22Queue, &dht_data, pdMS_TO_TICKS(100))==pdPASS) {
            humi_val = (uint32_t)dht_data->humidity;
            temp_val = (int32_t)dht_data->temperature;            
        } 

        
        
        // 第4行:光照 - 按照OLED规范，行号为1-4
        
        if (xQueueReceive(xLightQueue,&light_val,pdMS_TO_TICKS(100)) !=pdPASS)
        {
            light_val=0;   
        }

        if(lasthumi_val == humi_val&&lasttemp_val == temp_val&&lastlight_val == light_val)
        {
            continue;
        }
        OLED_Clear();
        // 第1行：标题
        OLED_ShowString(1, 1, "Sensor Monitor");        
        OLED_ShowString(2, 1, "Humi:");
        OLED_ShowString(3, 1, "Temp:");  // 修复：原来在这里用了错误的显示内容

        OLED_ShowNum(2, 7, humi_val, 2);
        OLED_ShowString(2, 9, "");
            
        OLED_ShowSignedNum(3, 7,temp_val, 3 );
        OLED_ShowString(3, 11, "C");     // 添加温度单位

        OLED_ShowString(4, 1, "Light:");
        OLED_ShowNum(4, 8, light_val, 4);

        lasthumi_val = humi_val;
        lasttemp_val = temp_val;
        lastlight_val = light_val;
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void printhello(void *arg)
{
    printf("hello world\n");
    while (1)
    {

        vTaskDelay(1000);
    }
}

// 光敏传感器任务 - 滑动窗口均值滤波（5次）
static void light_sensorTask(void *arg)
{
    LIGHT_Sensor_Init();
    uint16_t light_value = 0;
    uint16_t light_buf[5] = {0};
    uint8_t buf_idx = 0;
    uint8_t buf_count = 0;

    while (1)
    {
        light_value = LIGHT_Read_AO();

        light_buf[buf_idx] = light_value;
        buf_idx = (buf_idx + 1) % 5;
        if (buf_count < 5) buf_count++;

        if (buf_count == 5) {
            uint32_t sum = 0;
            for (uint8_t i = 0; i < 5; i++) {
                sum += light_buf[i];
            }
            uint16_t avg = (uint16_t)(sum / 5);
            xQueueSend(xLightQueue, &avg, portMAX_DELAY);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}




static void dht22Task(void *pvParameters)
{
    DHT22_Data *dht_data = pvPortMalloc(sizeof(DHT22_Data));

    float temp_buf[5] = {0};
    float humi_buf[5] = {0};
    uint8_t buf_idx = 0;
    uint8_t buf_count = 0;

    DHT22_Init();

    while(1) {
        DHT22_StartMeasurement();
        vTaskDelay(pdMS_TO_TICKS(2000));

        if(DHT22_Read(dht_data) == DHT22_OK) {
            temp_buf[buf_idx] = dht_data->temperature;
            humi_buf[buf_idx] = dht_data->humidity;
            buf_idx = (buf_idx + 1) % 5;
            if (buf_count < 5) buf_count++;

            if (buf_count == 5) {
                float temp_sum = 0.0f, humi_sum = 0.0f;
                for (uint8_t i = 0; i < 5; i++) {
                    temp_sum += temp_buf[i];
                    humi_sum += humi_buf[i];
                }
                dht_data->temperature = temp_sum / 5.0f;
                dht_data->humidity = humi_sum / 5.0f;
                xQueueSend(xDHT22Queue, &dht_data, portMAX_DELAY);
            }
        }
    }
}

static void usart_esp32Task(void *arg)
{
    USART2_ESP32_Init(115200);
    
    // 静态缓冲区（注意：如果任务被删除，这些内存会保留；这里任务永不删除，安全）
    static float temp_buffer[WINDOW_SIZE];
    static float humi_buffer[WINDOW_SIZE];
    static float light_buffer[WINDOW_SIZE];
    
    static AnomalyDetector temp_det, humi_det, light_det;
    AnomalyDetector_Init(&temp_det, temp_buffer, WINDOW_SIZE, ANOMALY_K);
    AnomalyDetector_Init(&humi_det, humi_buffer, WINDOW_SIZE, ANOMALY_K);
    AnomalyDetector_Init(&light_det, light_buffer, WINDOW_SIZE, ANOMALY_K);
    
    DHT22_Data *dht_data;
    uint16_t light_val;
    ESP32_Packet_t packet;
    
    while (1)
    {
        // 从队列中接收数据（阻塞等待直到有数据，或使用0超时轮询）
        // 注意：这里需要等待两个队列都有数据才发送，但为了实时性，可分别判断
        if (xQueuePeek(xDHT22Queue, &dht_data, pdMS_TO_TICKS(100)) == pdPASS &&
            xQueuePeek(xLightQueue, &light_val, 0) == pdPASS)
        {
            // 温度异常检测
            bool temp_anomaly = AnomalyDetector_Update(&temp_det, dht_data->temperature);
            // 湿度异常检测
            bool humi_anomaly = AnomalyDetector_Update(&humi_det, dht_data->humidity);
            // 光照异常检测（光照是 uint16_t，转为 float）
            bool light_anomaly = AnomalyDetector_Update(&light_det, (float)light_val);
            
            // 任意一个异常即置位
            packet.anomaly_flag = (temp_anomaly || humi_anomaly || light_anomaly) ? 1 : 0;
            
            // 填充数据包
            packet.data1 = dht_data->temperature;
            packet.data2 = dht_data->humidity;
            packet.data3 = light_val;
            
            // 发送到 ESP32
            ESP32_SendPacket(&packet);
        }
        // 任务周期约200ms（根据实际数据产生速率调整）
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
