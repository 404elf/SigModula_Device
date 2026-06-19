#include "VGA841.h"
#include "adc_measure.h"

#define rVREF 3.3f
#define rVgoal 1.5f
#define rVMAX 2.0f
#define VMAX (rVMAX/rVREF * 4095)   //  大概2482，VCA此时增益最大
#define Vgoal (rVgoal/rVREF * 4095-60) //留余量，原始大概1860
#define AGC_TARGET_AMP  Vgoal  //目标峰峰值
#define AGC_TOLERANCE   50.0f    //误差范围
#define VCA_DAC_LIMIT   VMAX     //限幅值

static uint16_t vca_dac_val = 1200; // 初始化中点约 1V
static uint32_t last_agc_ms = 0;    // AGC 200ms 节流计时

/**
  * @brief  AGC归一化幅值
  */
void Run_Slow_AGC(void) {
    float current_vpp = Get_Vpp();
    if (current_vpp < (rVgoal - 0.04f)) {
        vca_dac_val += 30;
    } else if (current_vpp > (rVgoal + 0.04f)) {
        vca_dac_val -= 30;
    }

    if (vca_dac_val > VCA_DAC_LIMIT) vca_dac_val = VCA_DAC_LIMIT;
    HAL_DAC_SetValue(&hdac, VCA_DAC_CHANNEL, DAC_ALIGN_12B_R, vca_dac_val);
}

/**
  * @brief  手动设置VCA841控制电压（绕过AGC反馈，直接给定增益）
  * @param  voltage: 目标电压 0 ~ 3.3V（rVREF）
  * @note   0V=最小增益, 2V=最大增益, 超过2V限幅
  */
void VGA_SetVoltage(float voltage) {
    if (voltage < 0.0f) voltage = 0.0f;
    if (voltage > rVMAX) voltage = rVMAX;
    vca_dac_val = (uint16_t)(voltage / rVREF * 4095);
    HAL_DAC_SetValue(&hdac, VCA_DAC_CHANNEL, DAC_ALIGN_12B_R, vca_dac_val);
}

/**
  * @brief  AGC （200ms一次，基于 HAL_GetTick）
  */
void Run_Slow_AGC_200ms(void) {
    uint32_t now = HAL_GetTick();
    if (now - last_agc_ms < 200) return;
    last_agc_ms = now;
    Run_Slow_AGC();
}

// /**
//   * @brief  慢速AGC
//   * @param  adc_raw_buffer: 存放adc采样的数组
//   * @param  buffer_len: 数组长度 (1024)
//   */
// void Run_Slow_AGC(uint16_t* adc_raw_buffer, uint32_t buffer_len) {
//     uint32_t i;
//     uint16_t adc_max = 0;
//     uint16_t adc_min = 4095;
//     float current_amp = 0.0f;

//     //找最小值最大值
//     for (i = 0; i < buffer_len; i++) {
//         if (adc_raw_buffer[i] > adc_max) adc_max = adc_raw_buffer[i];
//         if (adc_raw_buffer[i] < adc_min) adc_min = adc_raw_buffer[i];
//     }

//     //Vpp
//     current_amp = (float)(adc_max - adc_min);

//     //闭环反馈
//     if (current_amp < (AGC_TARGET_AMP - AGC_TOLERANCE)) {
//         //小了就增大（一次约 0.024V）
//         vca_dac_val += 30; 
//     } 
//     else if (current_amp > (AGC_TARGET_AMP + AGC_TOLERANCE)) {
//         //大了就减小
//         vca_dac_val -= 30;
//     }

//     //不能超过2V
//     if (vca_dac_val > VCA_DAC_LIMIT) {
//         vca_dac_val = VCA_DAC_LIMIT;
//     }

//     //电压写入VCA841
//     HAL_DAC_SetValue(&hdac, VCA_DAC_CHANNEL, DAC_ALIGN_12B_R, vca_dac_val);
// }

// /**
//   * @brief  慢速AGC — 200ms自动节流包装
//   * @param  adc_raw_buffer: 存放adc采样的数组
//   * @param  buffer_len: 数组长度 (1024)
//   */
// void Run_Slow_AGC_200ms(uint16_t* adc_raw_buffer, uint32_t buffer_len) {
//     uint32_t now = DWT_Get_ms();
//     if (now - last_agc_ms < 200) return;
//     last_agc_ms = now;
//     Run_Slow_AGC(adc_raw_buffer,buffer_len);
// }
