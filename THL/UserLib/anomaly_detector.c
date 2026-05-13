#include "anomaly_detector.h"
#include <math.h>

void AnomalyDetector_Init(AnomalyDetector *det, float *buf, uint16_t size, float k)
{
    det->buffer = buf;
    det->size = size;
    det->index = 0;
    det->count = 0;
    det->k = k;
    det->mean = 0.0f;
    det->stddev = 0.0f;
}

static void RecalcStats(AnomalyDetector *det)
{
    if (det->count == 0) {
        det->mean = 0.0f;
        det->stddev = 0.0f;
        return;
    }
    // 计算均值
    float sum = 0.0f;
    for (uint16_t i = 0; i < det->count; i++) {
        sum += det->buffer[i];
    }
    det->mean = sum / det->count;

    // 计算标准差
    float sq_sum = 0.0f;
    for (uint16_t i = 0; i < det->count; i++) {
        float diff = det->buffer[i] - det->mean;
        sq_sum += diff * diff;
    }
    det->stddev = sqrtf(sq_sum / det->count);
}

bool AnomalyDetector_Update(AnomalyDetector *det, float new_value)
{
    // 1. 判断是否异常（至少需要2个样本才能计算标准差，避免除零）
    bool is_anomaly = false;
    if (det->count >= 2) {
        float diff = new_value - det->mean;
        if (diff < 0) diff = -diff;
        if (diff > det->k * det->stddev) {
            is_anomaly = true;
        }
    }

    // 2. 将新值写入环形缓冲区
    det->buffer[det->index] = new_value;
    det->index = (det->index + 1) % det->size;
    if (det->count < det->size) {
        det->count++;
    }

    // 3. 重新统计（简单实现，可优化为增量更新，但工程上足够）
    RecalcStats(det);

    return is_anomaly;
}

float AnomalyDetector_GetMean(AnomalyDetector *det)
{
    return det->mean;
}

float AnomalyDetector_GetStddev(AnomalyDetector *det)
{
    return det->stddev;
}
