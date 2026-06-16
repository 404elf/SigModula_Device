#ifndef _FFT_H
#define _FFT_H	 

#include "main.h"
#include "arm_math.h"

// FFT采样点数（2^n）
#define FFT_LENGTH  1024  

// 函数声明
void FFT_Init(void);
void FFT_Calculate(float32_t* input_real_data, float32_t* output_imag_data);

#endif

