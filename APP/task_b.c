#include "task_b.h"
#include "app_main.h"

#define MAX_ZC 32

static float measured_mf   = 0.0f;   // 调频度
static float measured_dfm  = 0.0f;   // Δfm (kHz)
static float measured_freq = 0.0f;   // 调制频率 F (Hz)
static float measured_vpp  = 0.0f;

/* ---- 最大频偏 Δfm (kHz)，同时输出调制信号 Vpp (Hz→V映射) ---- */
static float Calc_DeltaFm(float* fm_dev, uint32_t len, float* mod_vpp) {
    float fmax = fm_dev[0];
    float fmin = fm_dev[0];
    for (uint32_t i = 1; i < len; i++) {
        if (fm_dev[i] > fmax) fmax = fm_dev[i];
        if (fm_dev[i] < fmin) fmin = fm_dev[i];
    }
    *mod_vpp = (fmax - fmin) * 3.3f / 60000.0f;  // 60kHz → 3.3V
    return (fmax - fmin) / 2000.0f;                // Δfm (kHz)
}

/* ---- 多周期过零测频（带迟滞抗噪） ---- */
static float Calc_Freq_ZC(float* signal, uint32_t len, float fs) {
    float mean = 0.0f;
    for (uint32_t i = 0; i < len; i++) mean += signal[i];
    mean /= (float)len;

    uint32_t zc_idx[MAX_ZC];
    uint32_t zc_cnt = 0;
    float hysteresis = 1000.0f;  // FM 频偏单位是 Hz，1kHz 死区挡微分毛刺
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

/**
  * @brief  任务B初始化 — 每次切换到B时执行
  */
void TaskB_Init(void) {
    AD985x_SetFre_Phase(9800000.0f, 0);
    VGA_SetVoltage(1.0f);
    SignalGen_Resume();

    OLED_ClearBuffer();
    OLED_ShowString(0, 0, "Task B: FM Demod", 1);
    OLED_ShowString(0, 2, "mf :", 1);
    OLED_ShowString(0, 4, "dfm:", 1);
    OLED_ShowString(0, 6, "F  :", 1);
    OLED_Refresh();
}

/**
  * @brief  任务B主循环
  */
void TaskB_Loop(void) {
    Run_Slow_AGC_200ms();

    if (!adc_buffer_ready) return;
    uint8_t buf = adc_buffer_ready - 1;
    adc_buffer_ready = 0;

    // IQ 解调 → FM_deviation_buffer（瞬时频偏，Hz）
    Run_IQ_Demodulation_800k(v_process_buffer[buf]);

    // Δfm (kHz)，同时得调制信号 Vpp
    measured_dfm  = Calc_DeltaFm(FM_deviation_buffer, FFT_LENGTH, &measured_vpp);

    // 过零测频（调制信号 3~5kHz）
    measured_freq = Calc_Freq_ZC(FM_deviation_buffer, FFT_LENGTH, 800000.0f);
    if (measured_freq < 3000.0f || measured_freq > 5000.0f)
        measured_freq = 0.0f;

    // mf = Δfm(Hz) / F = Δfm(kHz) * 1000 / F
    if (measured_freq > 0.0f)
        measured_mf = measured_dfm * 1000.0f / measured_freq;
    else
        measured_mf = 0.0f;

    // DDS 输出：幅度用调制频偏 Vpp，频率跟随 F
    if (measured_freq > 0.0f) {
        SignalGen_UpdateVpp(measured_vpp);
        Set_DDS_Freq(measured_freq);
    }

    // OLED 刷新
    OLED_ShowFloat(40, 2, measured_mf,  2, 2, 1);
    OLED_ShowFloat(40, 4, measured_dfm, 1, 2, 1);
    OLED_ShowFloat(40, 6, measured_freq, 1, 2, 1);
}

