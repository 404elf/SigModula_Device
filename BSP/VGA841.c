#include "VGA841.h"

#define rVREF 3.3f
#define rVgoal 1.5f
#define rVMAX 2.0f
#define VMAX (rVMAX/rVREF * 4095)   //  大概2482，VCA此时增益最大
#define Vgoal (rVgoal/rVREF * 4095-60) //留余量，原始大概1860
#define AGC_TARGET_AMP  Vgoal  //目标峰峰值
#define AGC_TOLERANCE   50.0f    //误差范围
#define VCA_DAC_LIMIT   VMAX     //限幅值

static uint16_t vca_dac_val = 1200; // 初始化中点约 1V

/**
  * @brief  慢速AGC(200ms一次)
  * @param  adc_raw_buffer: 存放adc采样的数组
  * @param  buffer_len: 数组长度 (1024)
  */
void Run_Slow_AGC(uint16_t* adc_raw_buffer, uint32_t buffer_len) {
    uint32_t i;
    uint16_t adc_max = 0;
    uint16_t adc_min = 4095;
    float current_amp = 0.0f;

    //找最小值最大值
    for (i = 0; i < buffer_len; i++) {
        if (adc_raw_buffer[i] > adc_max) adc_max = adc_raw_buffer[i];
        if (adc_raw_buffer[i] < adc_min) adc_min = adc_raw_buffer[i];
    }

    //Vpp
    current_amp = (float)(adc_max - adc_min);

    //闭环反馈
    if (current_amp < (AGC_TARGET_AMP - AGC_TOLERANCE)) {
        //小了就增大（一次约 0.024V）
        vca_dac_val += 30; 
    } 
    else if (current_amp > (AGC_TARGET_AMP + AGC_TOLERANCE)) {
        //大了就减小
        vca_dac_val -= 30;
    }

    //不能超过2V
    if (vca_dac_val > VCA_DAC_LIMIT) {
        vca_dac_val = VCA_DAC_LIMIT;
    }

    //电压写入VCA841
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, vca_dac_val);
}

