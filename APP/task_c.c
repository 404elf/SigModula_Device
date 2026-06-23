#include "task_c.h"
#include "app_main.h"
#include "arm_math.h"

#define MAX_ZC      32
#define NOISE_GATE  5000.0f        // Goertzel 能量阈值

static uint8_t  mode = 2;          // 0=AM, 1=FM, 2=CW
static float measured_ma   = 0.0f;
static float measured_mf   = 0.0f;
static float measured_dfm  = 0.0f;
static float measured_freq = 0.0f;
static float measured_vpp  = 0.0f;
static float current_fc    = 0.0f;  // 载波频率 (MHz)

/* ---- 标准差 ---- */
static float Calc_StdDiv(float* data, uint32_t len) {
    float mean = 0.0f;
    for (uint32_t i = 0; i < len; i++) mean += data[i];
    mean /= (float)len;
    float sum_sq = 0.0f;
    for (uint32_t i = 0; i < len; i++) {
        float d = data[i] - mean;
        sum_sq += d * d;
    }
    return sqrtf(sum_sq / (float)len);
}

/* ---- Goertzel 单频能量检测 @200kHz (fs=800k, N=1024 → k=256, coeff=0) ---- */
static float Goertzel_200k(float* raw, uint32_t len) {
    float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f;
    for (uint32_t i = 0; i < len; i++) {
        s0 = raw[i] - s2;       // coeff=0 → s[n] = x[n] - s[n-2]
        s2 = s1;
        s1 = s0;
    }
    return s1 * s1 + s2 * s2;   // |X[k]|^2
}

/* ---- 多周期过零测频（迟滞门限可配） ---- */
static float Calc_Freq_ZC(float* signal, uint32_t len, float fs, float hysteresis) {
    float mean = 0.0f;
    for (uint32_t i = 0; i < len; i++) mean += signal[i];
    mean /= (float)len;

    uint32_t zc_idx[MAX_ZC];
    uint32_t zc_cnt = 0;
    uint8_t state = 0;

    for (uint32_t i = 1; i < len && zc_cnt < MAX_ZC; i++) {
        if (state == 0 && (signal[i] > mean + hysteresis)) {
            state = 1;
            zc_idx[zc_cnt++] = i;
        } else if (state == 1 && (signal[i] < mean - hysteresis)) {
            state = 0;
        }
    }
    if (zc_cnt < 2) return 0.0f;

    uint32_t total_samples = zc_idx[zc_cnt - 1] - zc_idx[0];
    float period = (float)total_samples / (float)(zc_cnt - 1) / fs;
    return 1.0f / period;
}

/* ---- 调幅度 ma，同时输出包络 Vpp (V) ---- */
static float Calc_MA(float* envelope, uint32_t len, float* env_vpp) {
    float amax = envelope[0], amin = envelope[0];
    for (uint32_t i = 1; i < len; i++) {
        if (envelope[i] > amax) amax = envelope[i];
        if (envelope[i] < amin) amin = envelope[i];
    }
    *env_vpp = (amax - amin) * 3.3f / 4095.0f;
    if (amax + amin < 0.001f) return 0.0f;
    return (amax - amin) / (amax + amin);
}

/* ---- 最大频偏 Δfm (kHz)，同时输出调制 Vpp (Hz→V) ---- */
static float Calc_DeltaFm(float* fm_dev, uint32_t len, float* mod_vpp) {
    float fmax = fm_dev[0], fmin = fm_dev[0];
    for (uint32_t i = 1; i < len; i++) {
        if (fm_dev[i] > fmax) fmax = fm_dev[i];
        if (fm_dev[i] < fmin) fmin = fm_dev[i];
    }
    *mod_vpp = (fmax - fmin) * 3.3f / 60000.0f;
    return (fmax - fmin) / 2000.0f;
}

/* ================================================================
 *  阶段一：扫频锁相 (Carrier Sweep & Lock)
 * ================================================================ */
static uint8_t SweepAndLock(void) {
    for (uint8_t k = 0; k <= 40; k++) {
        float flo = 9.8f + 0.5f * (float)k;  // MHz
        AD985x_SetFre_Phase(flo * 1.0e6f, 0);

        // 等待 2ms 信号稳定
        uint32_t t0 = HAL_GetTick();
        while (HAL_GetTick() - t0 < 2);

        // 丢弃过渡期脏数据，强制等全新一帧
        adc_buffer_ready = 0;

        // 等待 ADC 乒乓缓冲就绪（超时 10ms）
        uint32_t timeout = HAL_GetTick();
        while (!adc_buffer_ready && (HAL_GetTick() - timeout < 10));
        if (!adc_buffer_ready) continue;

        uint8_t buf = adc_buffer_ready - 1;
        adc_buffer_ready = 0;

        float energy = Goertzel_200k(v_process_buffer[buf], FFT_LENGTH);
        if (energy > NOISE_GATE) {
            current_fc = flo + 0.2f;        // fc = fLO + IF
            return 1;
        }
    }
    current_fc = 0.0f;
    return 0;
}

/* ================================================================
 *  任务 C 初始化
 * ================================================================ */
void TaskC_Init(void) {
    VGA_SetVoltage(1.0f);
    SignalGen_Resume();

    // 扫频锁相
    uint8_t locked = SweepAndLock();

    // OLED 显示
    OLED_ClearBuffer();
    OLED_ShowString(0, 0, "Task C: Auto", 1);
    if (locked) {
        OLED_ShowFloat(0, 2, current_fc, 3, 1, 1);
        OLED_ShowString(0, 4, "Locked", 1);
    } else {
        OLED_ShowString(0, 2, "No Signal", 1);
    }
    OLED_Refresh();
}

/* ================================================================
 *  任务 C 主循环
 * ================================================================ */
void TaskC_Loop(void) {
    Run_Slow_AGC_200ms();

    if (!adc_buffer_ready) return;
    uint8_t buf = adc_buffer_ready - 1;
    adc_buffer_ready = 0;

    // IQ 解调 → AM_envelope_buffer + FM_deviation_buffer
    Run_IQ_Demodulation_800k(v_process_buffer[buf]);

    // 标准差
    float S_env  = Calc_StdDiv(AM_envelope_buffer, FFT_LENGTH);
    float S_freq = Calc_StdDiv(FM_deviation_buffer, FFT_LENGTH);

    // ---- 决策树 ----
    if (S_env > 100.0f) {
        // AM
        mode = 0;
        measured_ma   = Calc_MA(AM_envelope_buffer, FFT_LENGTH, &measured_vpp);
        measured_freq = Calc_Freq_ZC(AM_envelope_buffer, FFT_LENGTH, 800000.0f, 20.0f);
        if (measured_freq < 1000.0f || measured_freq > 10000.0f)
            measured_freq = 0.0f;
        measured_dfm  = 0.0f;
        measured_mf   = 0.0f;
    } else if (S_freq > 1500.0f) {
        // FM
        mode = 1;
        measured_dfm  = Calc_DeltaFm(FM_deviation_buffer, FFT_LENGTH, &measured_vpp);
        measured_freq = Calc_Freq_ZC(FM_deviation_buffer, FFT_LENGTH, 800000.0f, 1000.0f);
        if (measured_freq < 3000.0f || measured_freq > 10000.0f)
            measured_freq = 0.0f;
        if (measured_freq > 0.0f)
            measured_mf = measured_dfm * 1000.0f / measured_freq;
        else
            measured_mf = 0.0f;
        measured_ma   = 0.0f;
    } else {
        // CW
        mode = 2;
        measured_ma   = 0.0f;
        measured_mf   = 0.0f;
        measured_dfm  = 0.0f;
        measured_freq = 0.0f;
        measured_vpp  = 0.0f;
    }

    // DDS 输出：幅度用调制信号 Vpp，频率跟随 F
    if (measured_freq > 0.0f) {
        SignalGen_UpdateVpp(measured_vpp);
        Set_DDS_Freq(measured_freq);
    }

    // OLED 刷新
    OLED_ClearBuffer();
    OLED_ShowString(0, 0, mode == 0 ? "AM " : mode == 1 ? "FM " : "CW ", 1);
    OLED_ShowFloat(40, 0, current_fc, 3, 1, 1);
    if (mode == 0) {
        OLED_ShowFloat(0, 2, measured_ma,   2, 2, 1);
        OLED_ShowFloat(0, 4, measured_freq, 1, 2, 1);
    } else if (mode == 1) {
        OLED_ShowFloat(0, 2, measured_mf,   2, 2, 1);
        OLED_ShowFloat(0, 4, measured_dfm,  1, 2, 1);
        OLED_ShowFloat(0, 6, measured_freq, 1, 2, 1);
    } else {
        OLED_ShowFloat(0, 2, current_fc, 3, 1, 1);
        OLED_ShowString(40, 2, "MHz CW", 1);
    }
    OLED_Refresh();
}
