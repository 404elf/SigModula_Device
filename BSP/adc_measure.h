#ifndef __ADC_MEASURE_H
#define __ADC_MEASURE_H

#include "main.h"

/* ===== 移植配置：修改这两个宏即可适配不同 ADC 外设 ===== */
#define MEASURE_ADC_HANDLE   hadc1      // ADC 句柄（CubeMX 生成的变量名）
#define MEASURE_ADC_INSTANCE ADC1       // ADC 外设实例（用于回调中判断）

#ifndef FFT_LENGTH
#define FFT_LENGTH 1024
#endif
#define ADC_BUF_SIZE (FFT_LENGTH * 2)

extern volatile uint8_t adc_buffer_ready;   // 0=无, 1=半缓冲就绪, 2=全缓冲就绪
extern uint16_t ADC_Value_Buffer[ADC_BUF_SIZE];
extern float v_process_buffer[2][FFT_LENGTH];

void ADC_Measure_Start(void);   // 启动ADC测量
void ADC_Cal_Vpp(uint16_t* pBuffer, uint16_t length, uint8_t ping_pong_index);    // 计算Vpp峰峰值

float Get_Vpp(void);

void Measure_ADC_HalfCpltCallback(void);
void Measure_ADC_FullCpltCallback(void);

#endif
