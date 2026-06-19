#ifndef __VGA841_H
#define __VGA841_H	 

#include "main.h"
#include "dac.h"

#define VCA_DAC_CHANNEL  DAC_CHANNEL_1   // VCA 使用的 DAC 通道

void Run_Slow_AGC(uint16_t* adc_raw_buffer, uint32_t buffer_len);
void Run_Slow_AGC_200ms(uint16_t* adc_raw_buffer, uint32_t buffer_len);  // DWT定时200ms自动节流
void VGA_SetVoltage(float voltage);          // 手动设置控制电压 (0 ~ rVREF)

void DWT_Init(void);
uint32_t DWT_Get_ms(void);

#endif


