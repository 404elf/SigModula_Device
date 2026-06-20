#include "app_main.h"
#include "task_a.h"
#include "task_b.h"
#include "task_c.h"
#include "tim.h"

// 定义系统运行模式枚举
typedef enum {
    TASK_Init = 0,
    TaskA     = 1,
    TaskB     = 2,
    TaskC     = 3
} MachineTaskMode_t;

static volatile MachineTaskMode_t key_flag = TASK_Init;
static MachineTaskMode_t key_last_flag  = TASK_Init;
static uint8_t tab_flag = 0;
static uint32_t last_press_tick = 0;   // 按键去抖计时

//函数实现  
void app_init(void) {
    OLED_Init();
    FFT_Init();
    Init_SineRef();

    AD985x_Init(AD9850, SERIAL);
    AD985x_SetFre_Phase(9800000.0f, 0);

    ADC_Measure_Start();
    SignalGen_Start(0.0f);
    VGA_SetVoltage(1.0f);

    //开启时钟（顺手清零）
    __HAL_TIM_SET_COUNTER(&htim3,0);
    __HAL_TIM_SET_COUNTER(&htim6,0);
    HAL_TIM_Base_Start(&htim3);
    HAL_TIM_Base_Start(&htim6);

}


void app_loop(void) {
    static uint8_t init_once = 0;
    if (key_last_flag!=key_flag){
      key_last_flag=key_flag;
      tab_flag=1;
    }
    else{
      tab_flag=0;
    }
    switch (key_flag){
        case TASK_Init:
            if (!init_once) {
                OLED_ClearBuffer();
                OLED_ShowString(0, 0, "Press to start", 1);
                OLED_Refresh();
                init_once = 1;
            }
            break;
        case TaskA:
            if (tab_flag) TaskA_Init();
            TaskA_Loop();
            break;
        case TaskB:
            if (tab_flag) TaskB_Init();
            TaskB_Loop(); 
            break;
        case TaskC:
            if (tab_flag) TaskC_Init();
            TaskC_Loop(); 
            break;
        default: break;
    }
}

// 按键处理（EXIT 中断回调）
void Key_handler(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if (now - last_press_tick < 200) return;   // 200ms 去抖
    if (key_flag==TASK_Init) {key_flag = TaskA;return;}
    switch (GPIO_Pin) {
        case GPIO_PIN_0:
            // 上一个模式：TaskC → TaskB → TaskA → TaskC
            if (key_flag > TaskA) key_flag = (MachineTaskMode_t)(key_flag - 1);
            else                  key_flag = TaskC;
            break;

        case GPIO_PIN_1:
            // 下一个模式：TaskA → TaskB → TaskC → TaskA
            if (key_flag < TaskC) key_flag = (MachineTaskMode_t)(key_flag + 1);
            else                  key_flag = TaskA;
            break;

        case GPIO_PIN_3:
            key_flag = TaskC;
            break;

        default:
            return;   // 未匹配则不刷新 last_press_tick
    }

    last_press_tick = now;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    Key_handler(GPIO_Pin);
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

