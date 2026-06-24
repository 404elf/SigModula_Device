#ifndef __IQ_H
#define __IQ_H	 

#include "main.h"
#include "FFT.h"

// 解调后滑动平均窗口宽度
#define AM_SMOOTH_WINDOW  16U
#define FM_SMOOTH_WINDOW  16U

extern float AM_envelope_buffer[FFT_LENGTH];
extern float FM_deviation_buffer[FFT_LENGTH];

void Run_IQ_Demodulation_800k(float* pRawData);

#endif


