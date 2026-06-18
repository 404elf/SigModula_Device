#include "IQ.h"
#include "FFT.h"
#include "arm_math.h"

// 4点滑动平均滤波器缓冲区
static float filter_buf_I[4] = {0};
static float filter_buf_Q[4] = {0};
static uint8_t filter_index = 0;

// 1024点时域解调数组
float AM_envelope_buffer[FFT_LENGTH];   
float FM_deviation_buffer[FFT_LENGTH];  

/**
  * @brief：IQ 解调算法
  * @param：待解调数组（原始数据）
  */
void Run_IQ_Demodulation_800k(float* pRawData) {
    uint32_t n;
    static float prev_theta = 0.0f;
    float I_raw, Q_raw;
    float I_filtered, Q_filtered;
    float current_theta, delta_theta;

    //正交同相分量相乘，进行解调
    for (n = 0; n < FFT_LENGTH; n++) {
        uint8_t phase_step = n & 0x03; //n % 4（2^n专用）
        
        switch (phase_step) {
            case 0:
                I_raw =  2.0f * pRawData[n];
                Q_raw =  0.0f;
                break;
            case 1:
                I_raw =  0.0f;
                Q_raw = -2.0f * pRawData[n];
                break;
            case 2:
                I_raw = -2.0f * pRawData[n];
                Q_raw =  0.0f;
                break;
            case 3:
                I_raw =  0.0f;
                Q_raw =  2.0f * pRawData[n];
                break;
            default:
                I_raw = 0.0f;
                Q_raw = 0.0f;
                break;
        }

        //低通滤波，滤除高频400khz
        filter_buf_I[filter_index] = I_raw;
        filter_buf_Q[filter_index] = Q_raw;
        
        filter_index++;
        if (filter_index >= 4) filter_index = 0; // 4点循环

        // 四点滑动平均
        I_filtered = (filter_buf_I[0] + filter_buf_I[1] + filter_buf_I[2] + filter_buf_I[3]) / 4.0f;
        Q_filtered = (filter_buf_Q[0] + filter_buf_Q[1] + filter_buf_Q[2] + filter_buf_Q[3]) / 4.0f;

        //AM包络
        AM_envelope_buffer[n] = sqrtf(I_filtered * I_filtered + Q_filtered * Q_filtered);

        //FM 解调
        current_theta = atan2f(Q_filtered, I_filtered);

        // 相位解卷绕
        delta_theta = current_theta - prev_theta;
        if (delta_theta > PI)  delta_theta -= 2.0f * PI;
        if (delta_theta < -PI) delta_theta += 2.0f * PI;
        prev_theta = current_theta;

        // 差分求导 采样率为 fs = 800kHz
        // 常数项 800k/2pi ≈ 127323.954
        FM_deviation_buffer[n] = 127323.954f * delta_theta;
    }
}

