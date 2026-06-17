#ifndef __VGA841_H
#define __VGA841_H	 

#include "main.h"
#include "dac.h"

#define VCA_DAC_CHANNEL  DAC_CHANNEL_1   // VCA 使用的 DAC 通道

void Run_Slow_AGC(uint16_t* adc_raw_buffer, uint32_t buffer_len);

#endif


