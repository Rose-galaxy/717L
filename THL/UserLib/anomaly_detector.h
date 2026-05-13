#ifndef __ANOMALY_DETECTOR_H
#define __ANOMALY_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

// 滑动窗口异常检测器
typedef struct {
    float *buffer;          // 环形缓冲区
    uint16_t size;          // 窗口大小
    uint16_t index;         // 当前写入位置
    uint16_t count;         // 已存入的有效数据个数（< size 时未满）
    float k;                // 阈值系数（通常=3）
    float mean;             // 当前均值
    float stddev;           // 当前标准差
} AnomalyDetector;

// 初始化检测器（需外部提供缓冲区）
void AnomalyDetector_Init(AnomalyDetector *det, float *buf, uint16_t size, float k);

// 更新新值，返回 true 表示异常
bool AnomalyDetector_Update(AnomalyDetector *det, float new_value);

// 获取当前均值/标准差（可选）
float AnomalyDetector_GetMean(AnomalyDetector *det);
float AnomalyDetector_GetStddev(AnomalyDetector *det);

#endif
