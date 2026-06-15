#ifndef __AD985x_H
#define __AD985x_H	 
#include "main.h"

#define AD9851 			1
#define AD9850			0
 
#define PARALLEL		0 //并口
#define SERIAL			1 //串口

#define AD985x_FD_0    			0 //不倍频
#define AD985x_FD_6   			1 //使能6倍频

#define AD9850_CLK	125000000				//参考时钟
#define AD9851_CLK	180000000				//参考时钟

#define AD9850_f_Num  34.359738368	//频率转换系数：2^32/系统时钟频率
#define AD9851_f_Num  23.860929422	//频率转换系数：2^32/系统时钟频率

// AD985x 控制引脚定义 (STM32F4 HAL)
#define AD985x_GPIO_Port   GPIOA
#define AD985x_rst_Pin     GPIO_PIN_6   // 复位
#define AD985x_clk_Pin     GPIO_PIN_4   // 串行时钟输入
#define AD985x_fq_Pin      GPIO_PIN_3   // 数据更新

#define AD985x_Data_Port   GPIOB
#define AD985x_bit_data_Pin GPIO_PIN_7  // 串行数据输入

#define AD985x_DataBus     GPIOB->BSRR  // 并行数据总线 PB0-7 (BSRR 寄存器)

// AD985x 并行数据引脚 D0-D7 (PB0-PB7)
#define D0_Pin   GPIO_PIN_0
#define D1_Pin   GPIO_PIN_1
#define D2_Pin   GPIO_PIN_2
#define D3_Pin   GPIO_PIN_3
#define D4_Pin   GPIO_PIN_4
#define D5_Pin   GPIO_PIN_5
#define D6_Pin   GPIO_PIN_6
#define D7_Pin   GPIO_PIN_7

// 辅助宏 — 简化引脚操作
#define AD985x_rst_H()    HAL_GPIO_WritePin(AD985x_GPIO_Port, AD985x_rst_Pin, GPIO_PIN_SET)
#define AD985x_rst_L()    HAL_GPIO_WritePin(AD985x_GPIO_Port, AD985x_rst_Pin, GPIO_PIN_RESET)
#define AD985x_clk_H()    HAL_GPIO_WritePin(AD985x_GPIO_Port, AD985x_clk_Pin, GPIO_PIN_SET)
#define AD985x_clk_L()    HAL_GPIO_WritePin(AD985x_GPIO_Port, AD985x_clk_Pin, GPIO_PIN_RESET)
#define AD985x_fq_H()     HAL_GPIO_WritePin(AD985x_GPIO_Port, AD985x_fq_Pin, GPIO_PIN_SET)
#define AD985x_fq_L()     HAL_GPIO_WritePin(AD985x_GPIO_Port, AD985x_fq_Pin, GPIO_PIN_RESET)
#define AD985x_bit_data_H() HAL_GPIO_WritePin(AD985x_Data_Port, AD985x_bit_data_Pin, GPIO_PIN_SET)
#define AD985x_bit_data_L() HAL_GPIO_WritePin(AD985x_Data_Port, AD985x_bit_data_Pin, GPIO_PIN_RESET)
#define D0_H()            HAL_GPIO_WritePin(AD985x_Data_Port, D0_Pin, GPIO_PIN_SET)
#define D0_L()            HAL_GPIO_WritePin(AD985x_Data_Port, D0_Pin, GPIO_PIN_RESET)
#define D1_H()            HAL_GPIO_WritePin(AD985x_Data_Port, D1_Pin, GPIO_PIN_SET)
#define D1_L()            HAL_GPIO_WritePin(AD985x_Data_Port, D1_Pin, GPIO_PIN_RESET)
#define D2_H()            HAL_GPIO_WritePin(AD985x_Data_Port, D2_Pin, GPIO_PIN_SET)
#define D2_L()            HAL_GPIO_WritePin(AD985x_Data_Port, D2_Pin, GPIO_PIN_RESET)

extern uint8_t AD985x_FD; //倍频
extern uint8_t AD985x;			//要驱动的模块型号
extern uint8_t LOAD_MODE;	//串or并行驱动

//void AD985x_IO_Init(void);//初始化控制AD985x需要用到的IO口
void AD985x_Init(uint8_t cm ,uint8_t sp);//初始化控制AD985x需要用到的IO口及模式串并行加载模式

void AD985x_reset_parallel(void);//AD985x复位(并口模式)
void AD985x_reset_serial(void);	//AD985x复位(串口模式)

uint32_t Convert_Freq(double Real_f);//频率数据转换

void AD985x_wr_parrel(double fre,uint8_t phase,uint8_t mul);//向AD985x中写命令与数据(并口)
void AD985x_wr_serial(double fre,uint8_t phase,uint8_t mul);//向AD985x中写命令与数据(串口)
void AD985x_SetFre_Phase(double fre,uint8_t phase);//设置频率，相位



#endif

