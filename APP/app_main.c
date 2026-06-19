#include "app_main.h"


//º¯ÊýÊµÏÖ  
void app_init(void) {
    DWT_Init();
    OLED_Init();
    FFT_Init();
    Init_SineRef();

    AD985x_Init(AD9850, SERIAL);

    ADC_Measure_Start();
    SignalGen_Start(0.0f);
    VGA_SetVoltage(1.0f);
    
}

void app_loop(void) {
    Run_Slow_AGC_200ms(ADC_Value_Buffer,ADC_BUF_SIZE);
}




void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == MEASURE_ADC_INSTANCE) {
        Measure_ADC_HalfCpltCallback();
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == MEASURE_ADC_INSTANCE) {
        Measure_ADC_FullCpltCallback();
       
    }
}

void HAL_DAC_ConvHalfCpltCallbackCh2(DAC_HandleTypeDef *hdac) {
    if (hdac->Instance == DAC) {
        SignalGen_DAC_HalfCpltCallback();
    }
}

void HAL_DAC_ConvCpltCallbackCh2(DAC_HandleTypeDef *hdac) {
    if (hdac->Instance == DAC) {
        SignalGen_DAC_FullCpltCallback();
    }
}

