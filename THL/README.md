# STM32 智能环境监测系统项目文档

## 📋 项目概述

本项目是一个基于STM32F10x系列微控制器的智能环境监测系统，采用FreeRTOS实时操作系统实现多任务并发处理。系统能够实时采集温度、湿度和光照强度数据，通过滑动窗口算法进行异常检测，并通过ESP32模块将数据上传至云端或远程服务器。

### 主要功能特性

- ✅ **多传感器数据采集**：DHT22温湿度传感器 + 光敏电阻传感器
- ✅ **OLED实时显示**：128x64 OLED屏幕显示环境参数
- ✅ **异常检测算法**：基于滑动窗口的统计学异常检测（3σ原则）
- ✅ **无线通信**：通过USART2与ESP32模块通信，支持数据远程传输
- ✅ **FreeRTOS多任务调度**：5个独立任务协同工作
- ✅ **队列通信机制**：任务间通过消息队列安全传递数据

---

## 🏗️ 系统架构

### 硬件架构

```
┌─────────────────────────────────────────┐
│         STM32F10x 主控芯片              │
│                                         │
│  ┌──────────┐  ┌──────────┐            │
│  │  DHT22   │  │ 光敏传感器│            │
│  │ (PA0)    │  │ (PA5/PA6)│            │
│  └──────────┘  └──────────┘            │
│                                         │
│  ┌──────────┐  ┌──────────┐            │
│  │  OLED    │  │  ESP32   │            │
│  │ (I2C)    │  │ (USART2) │            │
│  └──────────┘  └──────────┘            │
│                                         │
│  ┌──────────┐                           │
│  │ LED指示  │ (PC13)                    │
│  └──────────┘                           │
└─────────────────────────────────────────┘
```

### 软件架构

```
┌──────────────────────────────────────────────┐
│           FreeRTOS 任务调度器                 │
├──────────┬──────────┬──────────┬─────────────┤
│ myTask   │dht22Task │light_    │usart_       │
│(LED闪烁) │(温湿度)  │sensorTask│esp32Task    │
│优先级:1  │优先级:3  │(光照)    │(数据发送)   │
│          │          │优先级:2  │优先级:2     │
├──────────┴──────────┴──────────┴─────────────┤
│         displayTask (OLED显示)                │
│         优先级:1                              │
├──────────────────────────────────────────────┤
│         消息队列 (Queue)                      │
│  xDHT22Queue  │  xLightQueue                 │
├──────────────────────────────────────────────┤
│         底层驱动库                             │
│  DHT22 | OLED | Light Sensor | USART2        │
│  Anomaly Detector | FreeRTOS Delay           │
└──────────────────────────────────────────────┘
```

---

## 📦 项目结构

```
THL/
├── Library/                  # STM32标准外设库
│   ├── inc/                 # 头文件
│   └── src/                 # 源文件
├── Start/                   # 启动文件和系统初始化
│   ├── core_cm3.c/h        # Cortex-M3核心文件
│   ├── stm32f10x.h         # 器件头文件
│   └── system_stm32f10x.c/h # 系统初始化
├── User/                    # 用户应用层
│   ├── main.c              # 主程序和任务定义 ⭐
│   ├── stm32f10x_conf.h    # 配置文件
│   └── stm32f10x_it.c/h    # 中断服务程序
├── UserLib/                 # 用户自定义驱动库
│   ├── DHT11.c/h           # DHT11传感器驱动（备用）
│   ├── dht22.c/h           # DHT22传感器驱动 ⭐
│   ├── OLED.c/h            # OLED显示驱动
│   ├── OLED_Font.h         # OLED字库
│   ├── light_sensor.c/h    # 光敏传感器驱动 ⭐
│   ├── anomaly_detector.c/h # 异常检测算法 ⭐
│   ├── usart_esp32.c/h     # ESP32通信协议 ⭐
│   ├── Serial.c/h          # 串口调试输出
│   ├── delay.c/h           # 延时函数
│   └── free_rtos_delay.c/h # FreeRTOS延时封装
├── freertos/                # FreeRTOS内核
│   ├── inc/                # 头文件
│   ├── src/                # 源文件
│   └── port/               # ARM CM3移植层
└── Objects/                 # 编译输出目录
```

---

## 🔧 硬件配置

### 引脚分配表

| 外设 | 引脚 | 功能说明 | 备注 |
|------|------|---------|------|
| **DHT22** | PA0 | 单总线数据引脚 | 需要上拉电阻4.7kΩ |
| **光敏传感器DO** | PA5 | 数字输出（阈值比较） | 可选使用 |
| **光敏传感器AO** | PA6 | 模拟输出（ADC1_IN6） | 主要使用此通道 |
| **OLED** | PB6/PB7 | I2C通信（SCL/SDA） | 需确认具体引脚 |
| **ESP32通信** | PA2/PA3 | USART2 TX/RX | 波特率115200 |
| **LED指示灯** | PC13 | 系统运行状态指示 | 板载LED |
| **调试串口** | PA9/PA10 | USART1 TX/RX | printf重定向 |

### 传感器规格

#### DHT22温湿度传感器
- **温度范围**：-40℃ ~ +80℃（精度±0.5℃）
- **湿度范围**：0% ~ 100%RH（精度±2%RH）
- **采样周期**：≥2秒
- **通信方式**：单总线协议

#### 光敏传感器
- **模拟输出**：0~3.3V（对应ADC值0~4095）
- **数字输出**：TTL电平（可通过电位器调节阈值）
- **响应波长**：可见光范围

#### OLED显示屏
- **分辨率**：128×64像素
- **接口**：I2C
- **驱动芯片**：SSD1306

---

## 💻 软件设计

### 任务列表

| 任务名称 | 优先级 | 栈大小 | 功能描述 | 周期 |
|---------|-------|--------|---------|------|
| `myTask` | 1 | 128 words | LED心跳指示 | 1秒翻转 |
| `dht22Task` | 3 | 256 words | DHT22数据采集 | 2秒/次 |
| `light_sensorTask` | 2 | 128 words | 光照数据采集 | 连续采样 |
| `displayTask` | 1 | 384 words | OLED数据显示 | 2秒刷新 |
| `usart_esp32Task` | 2 | 512 words | 数据打包发送 | 2秒/次 |
| `printhello` | 1 | 128 words | 串口调试输出 | 1秒/次 |

> **注意**：优先级数值越大表示优先级越高（FreeRTOS默认配置）

### 消息队列

#### xDHT22Queue
- **队列长度**：5
- **数据项大小**：`sizeof(DHT22_Data *)`
- **生产者**：`dht22Task`
- **消费者**：`displayTask`、`usart_esp32Task`
- **用途**：传递温湿度传感器数据指针

#### xLightQueue
- **队列长度**：5
- **数据项大小**：`sizeof(uint16_t)`
- **生产者**：`light_sensorTask`
- **消费者**：`displayTask`、`usart_esp32Task`
- **用途**：传递光照强度ADC值

### 异常检测算法

#### 算法原理
采用**滑动窗口统计方法**，基于3σ原则（三倍标准差）进行异常判定：

1. **维护滑动窗口**：保存最近N个数据点（默认N=20）
2. **计算统计量**：实时更新均值(μ)和标准差(σ)
3. **异常判定**：若新数据满足 `|x - μ| > k·σ`，则判定为异常（默认k=3.0）

#### 配置参数
```c
#define WINDOW_SIZE 20    // 滑动窗口大小
#define ANOMALY_K   3.0f  // 阈值系数（3σ原则）
```

#### 应用场景
- **温度异常**：突变检测（如火灾预警）
- **湿度异常**：环境突变监测
- **光照异常**：光线剧烈变化检测

### 通信协议

#### ESP32数据包结构
```c
typedef struct {
    float data1;           // 温度值（℃）
    float data2;           // 湿度值（%RH）
    uint16_t data3;        // 光照强度（ADC原始值）
    uint16_t anomaly_flag; // 异常标志：1=异常，0=正常
} ESP32_Packet_t;
```

**数据包大小**：14字节  
**传输方式**：USART2，115200波特率，8N1格式

---

## 🚀 系统工作流程

### 初始化流程

```
系统上电
  ↓
时钟使能（GPIOC）
  ↓
外设初始化
  ├─ Serial_Init()      // 调试串口
  ├─ OLED_Init()        // OLED显示
  └─ TIM2_Delay_Init()  // 延时定时器
  ↓
创建消息队列
  ├─ xDHT22Queue
  └─ xLightQueue
  ↓
创建FreeRTOS任务（6个任务）
  ↓
启动调度器 vTaskStartScheduler()
  ↓
进入多任务并发运行
```

### 数据流图

```
DHT22传感器 ──→ dht22Task ──→ xDHT22Queue ──┬──→ displayTask ──→ OLED显示
                                              └──→ usart_esp32Task ──→ ESP32

光敏传感器 ──→ light_sensorTask ──→ xLightQueue ──┬──→ displayTask ──→ OLED显示
                                                   └──→ usart_esp32Task ──→ ESP32
```

### 任务执行时序

```
时间轴 →
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
dht22Task:     [测量2s][读取][延迟2s][测量2s]...
light_task:    [采样][发送队列][采样][发送队列]...
displayTask:   [接收数据][刷新OLED][延迟2s]...
usart_task:    [接收数据][异常检测][打包发送][延迟2s]...
myTask:        [LED亮500ms][LED灭500ms]...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## 📊 关键代码说明

### 1. DHT22数据采集任务

```c
static void dht22Task(void *pvParameters)
{
    DHT22_Data *dht_data = pvPortMalloc(sizeof(DHT22_Data));
    DHT22_Init();
    
    while(1) {
        DHT22_StartMeasurement();        // 触发测量
        vTaskDelay(pdMS_TO_TICKS(2000));  // 等待2秒
        
        if(DHT22_Read(dht_data) == DHT22_OK) {
            xQueueSend(xDHT22Queue, &dht_data, portMAX_DELAY);
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));  // 间隔2秒
    }
}
```

**关键点**：
- 动态分配内存，避免栈溢出
- DHT22要求两次读取间隔至少2秒
- 使用阻塞式队列发送，确保数据不丢失

### 2. 异常检测与数据发送

```c
static void usart_esp32Task(void *arg)
{
    // 初始化三个独立的异常检测器
    static float temp_buffer[WINDOW_SIZE];
    static float humi_buffer[WINDOW_SIZE];
    static float light_buffer[WINDOW_SIZE];
    
    AnomalyDetector temp_det, humi_det, light_det;
    AnomalyDetector_Init(&temp_det, temp_buffer, WINDOW_SIZE, ANOMALY_K);
    // ... 其他初始化
    
    while (1) {
        if (xQueueReceive(xDHT22Queue, &dht_data, pdMS_TO_TICKS(100)) == pdPASS &&
            xQueueReceive(xLightQueue, &light_val, 0) == pdPASS) {
            
            // 分别检测三种数据的异常
            bool temp_anomaly = AnomalyDetector_Update(&temp_det, dht_data->temperature);
            bool humi_anomaly = AnomalyDetector_Update(&humi_det, dht_data->humidity);
            bool light_anomaly = AnomalyDetector_Update(&light_det, (float)light_val);
            
            // 任一异常即置位
            packet.anomaly_flag = (temp_anomaly || humi_anomaly || light_anomaly) ? 1 : 0;
            
            packet.data1 = dht_data->temperature;
            packet.data2 = dht_data->humidity;
            packet.data3 = light_val;
            
            ESP32_SendPacket(&packet);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

### 3. OLED显示任务

```c
static void displayTask(void *arg)
{
    while(1) {
        OLED_Clear();
        OLED_ShowString(1, 1, "Sensor Monitor");
        
        // 从队列获取最新数据（非阻塞）
        if(xQueueReceive(xDHT22Queue, &dht_data, 0) == pdPASS) {
            humi_val = (uint32_t)dht_data->humidity;
            temp_val = (int32_t)dht_data->temperature;
        }
        
        if (xQueueReceive(xLightQueue, &light_val, 0) != pdPASS) {
            light_val = 0;
        }
        
        // 显示格式化数据
        OLED_ShowString(2, 1, "Humi:");
        OLED_ShowNum(2, 7, humi_val, 2);
        
        OLED_ShowString(3, 1, "Temp:");
        OLED_ShowSignedNum(3, 7, temp_val, 3);
        OLED_ShowString(3, 11, "C");
        
        OLED_ShowString(4, 1, "Light:");
        OLED_ShowNum(4, 8, light_val, 4);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

---

## 🔍 调试与测试

### 串口调试输出

系统通过USART1提供调试信息：
- 启动时输出："hello world"
- 可用于printf打印调试信息

**串口配置**：
- 波特率：115200
- 数据位：8
- 停止位：1
- 校验位：无

### LED状态指示

- **PC13 LED**：每秒翻转一次，用于确认系统正常运行
- 如果LED停止闪烁，可能原因：
  - 系统崩溃或死锁
  - 看门狗复位
  - 电源问题

### 常见问题排查

| 问题现象 | 可能原因 | 解决方案 |
|---------|---------|---------|
| DHT22读取失败 | 接线错误或时序问题 | 检查PA0连接，确认上拉电阻 |
| OLED无显示 | I2C通信失败 | 检查SCL/SDA引脚，确认地址0x78 |
| 光照数据恒为0 | ADC未正确初始化 | 检查PA6连接和ADC配置 |
| ESP32无响应 | USART2配置错误 | 检查TX/RX交叉连接，波特率匹配 |
| 系统死机 | 栈溢出或优先级反转 | 增加任务栈大小，调整优先级 |

---

## 📈 性能优化建议

### 1. 内存管理
- **当前配置**：总栈大小约 1536 words = 6KB
- **建议**：监控`uxTaskGetStackHighWaterMark()`检查栈使用情况
- **优化**：根据实际使用情况调整各任务栈大小

### 2. 任务优先级调整
```
推荐优先级配置：
- dht22Task:        优先级3（最高，确保传感器及时读取）
- light_sensorTask: 优先级2
- usart_esp32Task:  优先级2
- displayTask:      优先级1（最低，显示可容忍延迟）
- myTask:           优先级1
```

### 3. 队列深度优化
- 当前队列深度为5，对于2秒周期的任务足够
- 如果提高采样频率，建议增加队列深度至10

### 4. 功耗优化（如需电池供电）
- 启用STM32低功耗模式
- 在任务空闲时调用`vTaskSuspend()`
- 降低OLED刷新频率至5秒/次

---

## 🛠️ 编译与烧录

### 开发环境
- **IDE**：Keil MDK-ARM / STM32CubeIDE
- **编译器**：ARMCC / GCC
- **调试器**：ST-Link V2 / J-Link

### 编译步骤
1. 打开工程文件（.uvprojx 或 .ioc）
2. 确认包含所有源文件路径
3. 编译项目（Build All）
4. 检查编译输出无错误和警告

### 烧录步骤
1. 连接ST-Link到开发板
2. 配置烧录器（Debug → Settings）
3. 下载程序到Flash
4. 复位运行

### 依赖库版本
- **STM32标准外设库**：V3.5.0
- **FreeRTOS**：V10.x
- **CMSIS**：V3.20

---

## 📝 扩展功能建议

### 短期改进
1. **增加数据存储**：添加SD卡模块记录历史数据
2. **报警功能**：检测到异常时蜂鸣器报警
3. **按键交互**：添加按键切换显示页面
4. **WiFi直连**：使用ESP32的AP模式建立Web服务器

### 长期规划
1. **云平台接入**：对接阿里云IoT/OneNet平台
2. **手机APP**：开发Android/iOS监控应用
3. **多节点组网**：多个监测节点通过LoRa/Zigbee组网
4. **机器学习**：在边缘端部署轻量级异常预测模型

---

## 📚 参考资料

1. **STM32F10x参考手册**：RM0008
2. **FreeRTOS官方文档**：https://www.freertos.org/
3. **DHT22数据手册**：Aosong官方规格书
4. **SSD1306 OLED驱动**：Solomon Systech datasheet
5. **ESP32 AT指令集**：Espressif官方文档

---

## 👥 项目信息

- **项目名称**：STM32智能环境监测系统
- **主控芯片**：STM32F10x系列（Cortex-M3）
- **实时系统**：FreeRTOS
- **开发语言**：C语言
- **最后更新**：2026-05-12

---

## 📄 许可证

本项目仅供学习和研究使用。

---

**文档版本**：v1.0  
**编写日期**：2026-05-12
