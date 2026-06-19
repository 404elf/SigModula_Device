#include "signal_gen.h"
#include "math.h"
#include "dac.h"
#include "tim.h"
#include "OLED.h"
//定周DDS   点准频率变  点就是波形精细度
//用指针来切表
//定频DDS   点变频率准
//一个表分上下部分流式输出

// 定时器触发DAC输出波形

// 内部变量 - 存储在内存中的双缓冲波形表

static uint16_t DAC_Buffer[DAC_BUF_SIZE] __attribute__((section(".sram2_data"))); // 唯一的一段环形 DMA 缓冲

#define REF_POINTS 1024
static float SineRef[REF_POINTS]; // 基准正弦表 (0.0~1.0)
static volatile float dds_phase = 0.0f;
static volatile float dds_phase_step = 0.0f; // 步长决定频率

//修改此即可修改vpp
static volatile float current_vpp_target = 0.0f;

/**
 * @brief 有着1024个点 的 基准sin表（值域为0，1）
 * REF_POINTS决定表有几个点
 */
void Init_SineRef(void) {
    for(int i=0; i<REF_POINTS; i++) 
        SineRef[i] = (sinf(2.0f * PI * i / REF_POINTS) + 1.0f) / 2.0f;
}


/**
 * @brief DDS流式数据块填充（填表，但是每次只填一半（看后面调用））
 * 根据不断累加的 dds_phase 动态计算下一个数据块
 */
static void DDS_Generate_Block(uint16_t* pBuffer, uint16_t length) {
    //current_vpp_target在这里起作用影响
    float amp = (current_vpp_target / 3.3f) * 4095.0f;
    float offset = 2048.0f - (amp / 2.0f);
    //上下皆做判断，可以确保从其它函数进来改变的情况不出错
    //读内存
    float dds_phase_Reg=dds_phase;
    for (int i = 0; i < length; i++) {
        // 防御性处理，防止内存越界
        if (dds_phase_Reg >= (float)REF_POINTS) dds_phase_Reg -= (float)REF_POINTS;
        if (dds_phase_Reg < 0) dds_phase_Reg = 0; 

        //线性插值法
        int index0 = (int)dds_phase_Reg;
        int index1 = index0 + 1;
        float frac = dds_phase_Reg - (float)index0;
        
        if (index1 >= REF_POINTS) index1 = 0;

        //插值核心
        float s0 = SineRef[index0];
        float s1 = SineRef[index1];
        float interpolated_sine = s0 + frac * (s1 - s0);  //距离决定权重

        float val = interpolated_sine * amp + offset;
        
        if (val > 4095.0f) val = 4095.0f;
        if (val < 0.0f) val = 0.0f;

        pBuffer[i] = (uint16_t)val;
        
        // 关键点：相位在此持续累加，绝对不会断层
        dds_phase_Reg += dds_phase_step;
        if (dds_phase_Reg >= (float)REF_POINTS) dds_phase_Reg -= (float)REF_POINTS;
    }
    //写内存
    dds_phase=dds_phase_Reg;
}

/**
 * @brief 系统启动波形输出(默认直流)
 */
void SignalGen_Start(float init_vpp) {
    HAL_DAC_Stop_DMA(&hdac, SIGGEN_DAC_CH);
    
    current_vpp_target = init_vpp;  //开始
    Set_DDS_Freq(0);
    dds_phase = 0.0f; // 重置初始相位

    // 预先填满整个 DMA 缓冲区
    DDS_Generate_Block(DAC_Buffer, DAC_BUF_SIZE);

    __HAL_TIM_SET_COUNTER(&SIGGEN_TIM_HANDLE,0);
    //先填充，后开DMA，避免毛刺
    HAL_DAC_Start_DMA(&hdac, SIGGEN_DAC_CH, (uint32_t*)DAC_Buffer, DAC_BUF_SIZE, DAC_ALIGN_12B_R);
    __HAL_TIM_SET_COUNTER(&SIGGEN_TIM_HANDLE,0);
}

/**
 * @brief 重新接管DAC DMA 控制权
 */
void SignalGen_Resume(void){
    HAL_DAC_Stop_DMA(&hdac, SIGGEN_DAC_CH);
    HAL_DAC_Start_DMA(&hdac, SIGGEN_DAC_CH, (uint32_t*)DAC_Buffer, DAC_BUF_SIZE, DAC_ALIGN_12B_R);
}


/**
 * @brief 重启波形输出
 */
void SignalGEN_Restart(void) {
    //表的第0位
    dds_phase = 0.0f; 
    __HAL_TIM_SET_COUNTER(&SIGGEN_TIM_HANDLE, 0); 
}

/**
 * @brief 修改Vpp。在运行时动态更新Vpp
 */
void SignalGen_UpdateVpp(float new_vpp) {
    // 现在更新 Vpp 非常简单！只需要改写这个变量。
    // CPU 在后台触发的 DMA 中断会实时读取这个新值，自动应用在下半段波形中。
    // 无需调用 __HAL_TIM_DISABLE，也无需 Stop_DMA！波形完美过渡。
    current_vpp_target = new_vpp;
}

//! 二编打卡，第二次看又忘记了。从等式变形的角度上看可能会更好
/**
 * @brief 修改频率。以1024个检查点为基准，判断一步要走几个检查点
 * @param freq 输出波形的频率
 */
void Set_DDS_Freq(float freq) {
    //DDS（Direct Digital Synthesis）直接数字频率合成
    //相位累加器
    //1000000.0f/freq 为在freq的波里一个周期能采x个点x dot/T，倒数就是 1/x T/dac_dot。每点占据1/x个周期
    //REF_POINTS是 y base_dot/T 每周期有y基准点
    //相乘 y/x base_dot/dac_dot 每个dac点对应几个基准点
    dds_phase_step = (freq / DAC_SAMPLE_RATE) * REF_POINTS;  
}

// 单表上下切换实现流式输出
// 当前半个缓冲被 DAC 发送出去时，触发回调
void SignalGen_DAC_HalfCpltCallback(void) {
    // DMA 正在发后半段，CPU 赶紧算出接下来的连续 DDS 点填入前半段
    // DAC是会自己输出的，所以这里是改变表的内容
    DDS_Generate_Block(&DAC_Buffer[0], DAC_BUF_SIZE / 2);
}

// 当后半个缓冲也被 DAC 发送出去时，触发回调
void SignalGen_DAC_FullCpltCallback(void) {
    // DMA 扭头去发前半段，CPU 赶紧算出接下来的连续 DDS 点填入后半段
    DDS_Generate_Block(&DAC_Buffer[DAC_BUF_SIZE / 2], DAC_BUF_SIZE / 2);
}
