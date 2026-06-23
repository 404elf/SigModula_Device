#include "IQ.h"
#include "FFT.h"
#include "arm_math.h"

// fs = 800kHz, IF = 200kHz → fs/IF = 4
// 2 * cos(2 * pi * IF / fs * n) = 2 * cos(pi * n / 2)
const float cos_table[4] = { 
    2.0f,   // n=0: 2 * cos(0)
    0.0f,   // n=1: 2 * cos(pi/2)
   -2.0f,   // n=2: 2 * cos(pi)
    0.0f    // n=3: 2 * cos(3pi/2)
};

// -2 * sin(2 * pi * IF / fs * n) = -2 * sin(pi * n / 2)
const float sin_table[4] = { 
    0.0f,   // n=0: -2 * sin(0)
   -2.0f,   // n=1: -2 * sin(pi/2)
    0.0f,   // n=2: -2 * sin(pi)
    2.0f    // n=3: -2 * sin(3pi/2)
};

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
        uint8_t phase_step = n & 0x03; // n % 4，用位运算加速查表
        
        I_raw = cos_table[phase_step] * pRawData[n];
        Q_raw = sin_table[phase_step] * pRawData[n];

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

        //离散频率偏移f_offset
        // 差分求导 采样率为 fs = 800kHz
        // 常数项 800k/2pi ≈ 127323.954
        FM_deviation_buffer[n] = 127323.954f * delta_theta;

    }
    //差分是高通滤波器，导致高频噪音被放大，现在用低通滤波
        float fm_sum = 0.0f;
        uint32_t i;

        // 初始16个点
        for (i = 0; i < 16; i++) {
            fm_sum += FM_deviation_buffer[i];
        }

        // 滑动窗口平均
        for (i = 0; i < FFT_LENGTH - 16; i++) {
            float avg = fm_sum / 16.0f;
            // 加上新进窗口的点，减去离开窗口的点
            fm_sum += FM_deviation_buffer[i + 16] - FM_deviation_buffer[i];
            // 将平滑后的值覆盖回原数组
            FM_deviation_buffer[i] = avg;
        }

        //补齐
        float last_val = FM_deviation_buffer[FFT_LENGTH - 17];
        for (i = FFT_LENGTH - 16; i < FFT_LENGTH; i++) {
            FM_deviation_buffer[i] = last_val;
        }
}

