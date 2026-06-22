#include "AD985x.h"

uint8_t AD985x_FD; //倍频
uint8_t AD985x;			//要驱动的模块型号
uint8_t LOAD_MODE;	//串or并行驱动

/***************************************************************************
** 函数名称 ：void AD985x_Init(uint8_t AD985x ,uint8_t Load_Mode)
** 函数功能 ：初始化控制AD985x需要用到的IO口及模式串并行加载模式
** 入口参数 ：cm：要驱动的型号，AD9850;AD9851
**						sp：驱动方式，PARALLEL并行驱动；SERIAL串行驱动
** 出口参数 ：无
** 函数说明 ：无
*****************************************************************************/
void AD985x_Init(uint8_t cm ,uint8_t sp)
{
	AD985x=cm;
	LOAD_MODE=sp;
	
	//AD985x_IO_Init();
	AD985x_DataBus=0xFFFF0000;	//低十六位全部拉低

	if(AD985x==AD9850) //AD9850
		AD985x_FD = AD985x_FD_0;//不倍频
	else	if(AD985x==AD9851)//AD9851
		AD985x_FD = AD985x_FD_6;//使能6倍频
	
	if(LOAD_MODE==PARALLEL)//并口
	{
		AD985x_reset_parallel();//复位为(并口模式)  
	}
	else if(LOAD_MODE==SERIAL) //串口
	{
		//D0_H();D1_H();D2_L();	//当使用串行驱动时，D0、D1、D2脚需输入（110）固定电平。
		AD985x_reset_serial();//复位为(串口模式)  
	}	
}

/***************************************************************************
** 函数名称 ：void AD985x_IO_Init(void)
** 函数功能 ：初始化控制AD985x需要用到的IO口
** 入口参数 ：无
** 出口参数 ：无
** 函数说明 ：无
*****************************************************************************/
/*~~
void AD985x_IO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure ; 

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC, ENABLE);	 //使能PA,PC端口时钟

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0| GPIO_Pin_1| GPIO_Pin_2| GPIO_Pin_3| GPIO_Pin_4| GPIO_Pin_5| GPIO_Pin_6| GPIO_Pin_7 ; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
	GPIO_Init(GPIOC ,&GPIO_InitStructure) ;

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3|GPIO_Pin_4| GPIO_Pin_6; 
	GPIO_Init(GPIOA ,&GPIO_InitStructure) ;
}
~~ */


/***************************************************************************
** 函数名称 ：void AD985x_reset_parallel()
** 函数功能 ：AD985x复位(并口模式)
** 入口参数 ：无
** 出口参数 ：无
** 函数说明 ：无
*****************************************************************************/
void AD985x_reset_parallel(void)
{
	AD985x_clk_L();
	AD985x_fq_L();
	//rest信号
	AD985x_rst_L();
	AD985x_rst_H();
	AD985x_rst_L();
}

/***************************************************************************
** 函数名称 ：void AD985x_reset_parallel()
** 函数功能 ：AD985x复位(串口模式)
** 入口参数 ：无
** 出口参数 ：无
** 函数说明 ：无
*****************************************************************************/
void AD985x_reset_serial(void)
{
	AD985x_clk_L();
	AD985x_fq_L();
	//rest信号
	AD985x_rst_L();
	AD985x_rst_H();
	AD985x_rst_L();
	//w_clk信号
	AD985x_clk_L();
	AD985x_clk_H();
	AD985x_clk_L();
	//fq_up信号
	AD985x_fq_L();
	AD985x_fq_H();
	AD985x_fq_L();
}

/************************************************************
** 函数名称 ：uint32_t Convert_Freq(double Real_f)
** 函数功能 ：频率数据转换
** 入口参数 ：Real_f，需要转换的频率
** 出口参数 ：频率数据值
** 函数说明 ：
**************************************************************/
uint32_t Convert_Freq(double Real_f)
{
	if(AD985x==AD9850) //AD9850
		return (uint32_t)(AD9850_f_Num*Real_f);
	else	if(AD985x==AD9851)//AD9851
		return (uint32_t)(AD9851_f_Num*Real_f);
	return 0;
}

/***************************************************************************
** 函数名称 ：void AD985x_wr_parrel(double fre,uint8_t phase,uint8_t mul)
** 函数功能 ： 向AD985x中写命令与数据(并口)
** 入口参数 ：fre: 频率
							phase:相位 0-31 ，一个控制字代表11.25°
							mul:倍频 AD9850芯片不支持倍频
** 出口参数 ：无
** 函数说明 ：无
*****************************************************************************/
void AD985x_wr_parrel(double fre,uint8_t phase,uint8_t mul)
{
	uint32_t w,y;
	
	y=Convert_Freq(fre);
	
	w=(phase<<3)|mul;   
	AD985x_DataBus=w|(w^0xff)<<16;     //写w0数据
	AD985x_clk_H();
	AD985x_clk_L();

	w=(uint8_t)(y>>24);
	AD985x_DataBus=w|(w^0xff)<<16;     //w1
	AD985x_clk_H();
	AD985x_clk_L();

	w=(uint8_t)(y>>16);
	AD985x_DataBus=w|(w^0xff)<<16;     //w2
	AD985x_clk_H();
	AD985x_clk_L();

	w=(uint8_t)(y>>8);
	AD985x_DataBus=w|(w^0xff)<<16;     //w3
	AD985x_clk_H();
	AD985x_clk_L();

	w=(uint8_t)(y);
	AD985x_DataBus=w|(w^0xff)<<16;     //w4
	AD985x_clk_H();
	AD985x_clk_L();
	
	//移入始能
	AD985x_fq_H();
	AD985x_fq_L();
}

/***************************************************************************
** 函数名称 ：void AD985x_wr_serial(double fre,uint8_t phase,uint8_t mul)
** 函数功能 ： 向AD985x中写命令与数据(串口)
** 入口参数 ：fre: 频率
							phase:相位 0-31 ，一个控制字代表11.25°
							mul:倍频 AD9850芯片不支持倍频
** 出口参数 ：无
** 函数说明 ：无
*****************************************************************************/
void AD985x_wr_serial(double fre,uint8_t phase,uint8_t mul)
{
	uint8_t i,w;
	uint32_t y;
	
	y=Convert_Freq(fre);
	//写w4数据
	w=(y>>=0);
	for(i=0;i<8;i++)
	{
		if((w>>i)&0x01) AD985x_bit_data_H(); else AD985x_bit_data_L();
		AD985x_clk_H();
		AD985x_clk_L();
	}
	//写w3数据
	w=(y>>8);
	for(i=0;i<8;i++)
	{
		if((w>>i)&0x01) AD985x_bit_data_H(); else AD985x_bit_data_L();
		AD985x_clk_H();
		AD985x_clk_L();
	}
	//写w2数据
	w=(y>>16);
	for(i=0;i<8;i++)
	{
		if((w>>i)&0x01) AD985x_bit_data_H(); else AD985x_bit_data_L();
		AD985x_clk_H();
		AD985x_clk_L();
	}
	//写w1数据
	w=(y>>24);
	for(i=0;i<8;i++)
	{
		if((w>>i)&0x01) AD985x_bit_data_H(); else AD985x_bit_data_L();
		AD985x_clk_H();
		AD985x_clk_L();
	}
	//写w0数据
	w=(phase<<3)|mul;  
	for(i=0;i<8;i++)
	{
		if((w>>i)&0x01) AD985x_bit_data_H(); else AD985x_bit_data_L();
		AD985x_clk_H();
		AD985x_clk_L();
	}
	//移入始能
	AD985x_fq_H();
	AD985x_fq_L();
}

/***************************************************************************
** 函数名称 ：void AD985x_Setfq(double fre,uint8_t phase)
** 函数功能 ：设置AD985x频率及相位
** 入口参数 ：fre: 频率
							phase:相位 0-31 ，一个控制字代表11.25°
** 出口参数 ：无
** 函数说明 ：无
*****************************************************************************/
void AD985x_SetFre_Phase(double fre,uint8_t phase)
{	
	if(LOAD_MODE==PARALLEL)
		AD985x_wr_parrel(fre,phase,AD985x_FD);
	else if(LOAD_MODE==SERIAL)
		AD985x_wr_serial(fre,phase,AD985x_FD);
}










































