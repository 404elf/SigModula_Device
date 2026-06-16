#include "FFT.h"

// 声明结构体（自定义）
static arm_rfft_fast_instance_f32 rfft_instance;
//存放结果
static float32_t fft_output_buffer[FFT_LENGTH];

/**
  * @brief  实数 RFFT 模块初始化
  */
void FFT_Init(void) {
    // 初始化快速实数 FFT，指定长度为 1024 点
    arm_rfft_fast_init_f32(&rfft_instance, FFT_LENGTH);
}
//markdown:时域数据->转复数时域形式（根据需要）->转频谱（实数+复数）（幅度+相位）->转幅度谱（纯幅度）
/**
  * @brief  实数 RFFT 计算
  * @param  input_real_data: 输入纯实数采样电压数组（长度必须为 FFT_LENGTH）
  * @param  output_mag_data: 输出的幅度谱，长度为 FFT_LENGTH/2 + 1
  */
void FFT_Calculate(float32_t* input_real_data, float32_t* output_mag_data) {
    uint32_t i;
    float32_t mean = 0.0f;

    //平均去直流
    for (i = 0; i < FFT_LENGTH; i++) {
        mean += input_real_data[i];
    }
    mean /= (float32_t)FFT_LENGTH;

    // 将 input_real_data 作为去直流的临时缓冲区
    //相比于先前，不需要转复数数组了，所以直接在输入基础上修改即可
    for (i = 0; i < FFT_LENGTH; i++) {
        input_real_data[i] -= mean;
    }
 
    // rfft，比cfft快一倍（必然自然序）
    //对称性，直接省一半长度
    arm_rfft_fast_f32(&rfft_instance, input_real_data, fft_output_buffer, 0);

    //幅度谱
    // 避坑0和奈奎斯特频率分量，虚部必然为 0
    //这里背ARM省去，直接把两点的实部放在最前面，所以需要手动处理
    output_mag_data[0] = fabsf(fft_output_buffer[0]);
    output_mag_data[FFT_LENGTH / 2] = fabsf(fft_output_buffer[1]);

    // 对于中间的复数频点段（从 1 到 N/2-1），调用库函数快速求模值
    // 起始地址设为 &fft_output_buffer[2]，输出写到幅度数组的 &output_mag_data[1] 处
    // 计算的复数个数为 (FFT_LENGTH / 2) - 1
    arm_cmplx_mag_f32(&fft_output_buffer[2], &output_mag_data[1], (FFT_LENGTH / 2) - 1);
}

//
// // 库结构体（从外导入，名字固定，储存表格，用于计算）
// extern const arm_cfft_instance_f32 arm_cfft_sR_f32_len1024;

// // 复数数据（长度2*FFT_LENGTH）
// static float32_t fft_complex_buffer[FFT_LENGTH * 2];

// /**
//   * @brief  FFT 模块初始化
//   */
// void FFT_Init(void) {
//     //加窗保留
// }

// /**
//   * @brief  FFT计算
//   * @param  input_real_data: 输入电压数值（长度为FFT_LENGTH）
//   * @param  output_mag_data: 输出的幅度谱谱线数组（长度为 FFT_LENGTH / 2）
//   */
// void FFT_Calculate(float32_t* input_real_data, float32_t* output_mag_data) {
//     uint32_t i;
//     float32_t mean = 0.0f;

//     //平均去直流
//     for (i = 0; i < FFT_LENGTH; i++) {
//         mean += input_real_data[i];
//     }
//     mean /= (float32_t)FFT_LENGTH;

//     //将电压实数数据转为FFT复数数据所需要数据
//     for (i = 0; i < FFT_LENGTH; i++) {
//         fft_complex_buffer[2 * i]     = input_real_data[i] - mean; // 实部（去平均）
//         fft_complex_buffer[2 * i + 1] = 0.0f;                      // 虚部0
//     }

//     //长度结构体结构体 输入输出复数缓冲区 0，1正逆变换 1反转输出使能（自然序，否则乱）
//     arm_cfft_f32(&arm_cfft_sR_f32_len1024, fft_complex_buffer, 0, 1);

//     // 计算复数的模值（幅度谱）
//     // 输出的有效谱线长度为 FFT_LENGTH / 2（根据 Nyquist 采样定理，后半部分是对称的重复数据，舍弃）
//     arm_cmplx_mag_f32(fft_complex_buffer, output_mag_data, FFT_LENGTH / 2);
// }


