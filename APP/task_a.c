#include "task_a.h"
#include "app_main.h"

#define MAX_ZC 32   // 过零检测最大点数

static float measured_ma   = 0.0f;
static float measured_freq = 0.0f;
static float measured_vpp  = 0.0f;

/* ---- 调幅度 ma = (Amax-Amin)/(Amax+Amin)，同时输出包络 Vpp (V) ---- */
static float Calc_MA(float* envelope, uint32_t len, float* env_vpp) {
    float amax = envelope[0];
    float amin = envelope[0];
    for (uint32_t i = 1; i < len; i++) {
        if (envelope[i] > amax) amax = envelope[i];
        if (envelope[i] < amin) amin = envelope[i];
    }
    float ma;
    if (amax + amin < 0.001f)
        ma = 0.0f;
    else
        ma = (amax - amin) / (amax + amin);
    *env_vpp = 2.0f * ma;  // 2V 满量程 × ma
    return ma;
}

/* ---- 多周期过零测频（带迟滞抗噪，最低支持1kHz） ---- */
static float Calc_Freq_ZC(float* envelope, uint32_t len, float fs) {
    float mean = 0.0f;
    for (uint32_t i = 0; i < len; i++) mean += envelope[i];
    mean /= (float)len;

    uint32_t zc_idx[MAX_ZC];
    uint32_t zc_cnt = 0;

    // 迟滞死区：消灭均值线附近的噪声抖动
    float hysteresis = 20.0f;
    uint8_t state = 0;  // 0=负半周, 1=正半周

    for (uint32_t i = 1; i < len && zc_cnt < MAX_ZC; i++) {
        if (state == 0 && (envelope[i] > mean + hysteresis)) {
            state = 1;
            zc_idx[zc_cnt++] = i;
        } else if (state == 1 && (envelope[i] < mean - hysteresis)) {
            state = 0;
        }
    }

    // 1kHz 在 1.28ms 窗口仅 1.28 周期，最多 2 次过零，改为 < 2
    if (zc_cnt < 2) return 0.0f;

    uint32_t total_samples = zc_idx[zc_cnt - 1] - zc_idx[0];
    float period = (float)total_samples / (float)(zc_cnt - 1) / fs;
    return 1.0f / period;
}

/**
 * @brief  任务A初始化 — 每次切换到A时执行，复位外设到A所需状态
 */
void TaskA_Init(void) {
    // AD9850 本振 9.8MHz（载波10MHz → IF=200kHz）
    AD985x_SetFre_Phase(9800000.0f, 0);

    // VGA 初始增益
    VGA_SetVoltage(1.0f);

    // 确保 DAC 波形输出在跑
    SignalGen_Resume();

    // OLED 任务标题
    OLED_ClearBuffer();
    OLED_ShowString(0, 0, "Task 1: AM Demod", 1);
    OLED_ShowString(0, 2, "ma :", 1);
    OLED_ShowString(0, 4, "F  :", 1);
    OLED_Refresh();
}

/**
 * @brief  任务A主循环
 */
void TaskA_Loop(void) {
    // 200ms AGC 自动节流
    Run_Slow_AGC_200ms();

    // 等待 ADC 乒乓缓冲就绪
    if (!adc_buffer_ready) return;
    uint8_t buf = adc_buffer_ready - 1;   // 1→buf[0], 2→buf[1]
    adc_buffer_ready = 0;

    // IQ 解调 → 包络数组 AM_envelope_buffer
    Run_IQ_Demodulation_800k(v_process_buffer[buf]);

    // 调幅度 ma（同时得调制包络 Vpp）
    measured_ma  = Calc_MA(AM_envelope_buffer, FFT_LENGTH, &measured_vpp);

    // 过零测频，滤除无效杂散（对齐 800~4000Hz）
    measured_freq = Calc_Freq_ZC(AM_envelope_buffer, FFT_LENGTH, 800000.0f);
    if (measured_freq < 500.0f || measured_freq > 6000.0f)
        measured_freq = 0.0f;

    // DDS 输出：幅度用 measured_vpp（= 2V × ma），频率跟随 F
    if (measured_freq > 0.0f) {
        SignalGen_UpdateVpp(measured_vpp);
        Set_DDS_Freq(measured_freq);
    }

    // OLED 刷新
    OLED_ClearBuffer();
    OLED_ShowString(0, 0, "Task 1: AM Demod", 1);
    OLED_ShowString(0, 2, "ma :", 1);
    OLED_ShowString(0, 4, "F  :", 1);
    OLED_ShowString(0, 6, "Vpp  :", 1);
    OLED_ShowFloat(40, 2, measured_ma,   2, 2, 1);
    OLED_ShowFloat(40, 4, measured_freq, 1, 2, 1);
    OLED_ShowFloat(40, 6, measured_vpp , 1, 2, 1);
    OLED_Refresh();
}
