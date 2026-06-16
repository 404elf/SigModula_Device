#ifndef __OLED_H
#define __OLED_H	 

#include "main.h"

/*==================================================================*
 *                         OLED 参数配置                             *
 *  用户可根据需要修改以下宏定义                                     *
 *==================================================================*/

// ---------- OLED 型号选择 ----------
#define OLED_128_64     1   // 分辨率 128×64（默认）
#define OLED_128_32     0   // 分辨率 128×32

// ---------- OLED I2C 引脚定义（GPIO 模拟 I2C） ----------
// 用户按需修改下面的引脚
#define OLED_I2C_SCL_Port   GPIOB
#define OLED_I2C_SCL_Pin    GPIO_PIN_6
#define OLED_I2C_SDA_Port   GPIOB
#define OLED_I2C_SDA_Pin    GPIO_PIN_7

// ---------- I2C 通信配置 ----------
#define OLED_I2C_ADDR       (0x78 >> 1)     // SSD1306 I2C 地址（7位地址，默认 0x3C 左移一位为 0x78）
#define OLED_I2C_SPEED_DELAY 5              // I2C 延时（单位：空循环次数，值越大速度越慢）

// ---------- GPIO 初始化开关 ----------
// 1 : OLED_Init() 自动配置 SCL/SDA 为推挽输出（方便快捷，但会和 CubeMX 配置冲突）
// 0 : 不自动配置，需用户在 CubeMX 中将 SCL/SDA 设为推挽输出（推荐，避免冲突）
#define OLED_GPIO_INIT_ENABLE   0           // 默认关闭，由 CubeMX 配置引脚

// ---------- 引脚操作宏（AD985x 风格） ----------
#define OLED_SCL_H()    HAL_GPIO_WritePin(OLED_I2C_SCL_Port, OLED_I2C_SCL_Pin, GPIO_PIN_SET)
#define OLED_SCL_L()    HAL_GPIO_WritePin(OLED_I2C_SCL_Port, OLED_I2C_SCL_Pin, GPIO_PIN_RESET)
#define OLED_SDA_H()    HAL_GPIO_WritePin(OLED_I2C_SDA_Port, OLED_I2C_SDA_Pin, GPIO_PIN_SET)
#define OLED_SDA_L()    HAL_GPIO_WritePin(OLED_I2C_SDA_Port, OLED_I2C_SDA_Pin, GPIO_PIN_RESET)
#define OLED_SDA_READ() HAL_GPIO_ReadPin(OLED_I2C_SDA_Port, OLED_I2C_SDA_Pin)

// ---------- I2C 读写操作宏 ----------
#define OLED_WriteCmd(cmd)    OLED_I2C_Write(0x00, (cmd))
#define OLED_WriteData(data)  OLED_I2C_Write(0x40, (data))

// ---------- 显示参数 ----------
#if OLED_128_64
    #define OLED_WIDTH      128
    #define OLED_HEIGHT     64
    #define OLED_PAGES      8               // 64 / 8
#elif OLED_128_32
    #define OLED_WIDTH      128
    #define OLED_HEIGHT     32
    #define OLED_PAGES      4               // 32 / 8
#endif

// ---------- 旋转方向 ----------
#define OLED_ROTATE_0       0               // 正常方向
#define OLED_ROTATE_180     1               // 旋转 180°

#ifndef OLED_ROTATE
    #define OLED_ROTATE     OLED_ROTATE_0   // 默认正常方向
#endif

//==================================================================*
//                         字体大小选择                              *
//==================================================================*
#define OLED_FONT_6X8       0               // 6×8 字体（默认）
#define OLED_FONT_8X16      1               // 8×16 字体

//==================================================================*
//                         全局变量 / 缓冲区                         *
//==================================================================*
extern uint8_t OLED_Buffer[OLED_WIDTH * OLED_PAGES];  // 显存缓冲区

//==================================================================*
//                         函数声明                                  *
//==================================================================*/

/**
  * @brief  OLED 初始化
  * @note   配置 SSD1306，根据 OLED_GPIO_INIT_ENABLE 决定是否自动初始化 GPIO
  *         若 OLED_GPIO_INIT_ENABLE = 0（默认），需在 CubeMX 中将 SCL/SDA 配为推挽输出
  */
void OLED_Init(void);

/**
  * @brief  将显存缓冲区内容刷新到 OLED
  * @note   每次修改完缓冲区后调用此函数更新屏幕
  */
void OLED_Refresh(void);

/**
  * @brief  清空显存缓冲区（不清屏，需调用 OLED_Refresh 才生效）
  */
void OLED_ClearBuffer(void);

/**
  * @brief  全屏填充
  * @param  data  填充数据（0x00 黑，0xFF 白）
  */
void OLED_Fill(uint8_t data);

/**
  * @brief  开启 / 关闭 OLED 显示
  * @param  on  0 关闭显示，1 开启显示
  */
void OLED_DisplayOn(uint8_t on);

/**
  * @brief  设置对比度
  * @param  contrast  0x00 ~ 0xFF
  */
void OLED_SetContrast(uint8_t contrast);

/**
  * @brief  画一个像素点
  * @param  x     x 坐标（0 ~ OLED_WIDTH - 1）
  * @param  y     y 坐标（0 ~ OLED_HEIGHT - 1）
  * @param  color 0 熄灭，1 点亮
  */
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);

/**
  * @brief  显示单个 ASCII 字符
  * @param  x      列起始坐标
  * @param  y      页坐标
  * @param  ch     要显示的字符
  * @param  font   字体：OLED_FONT_6X8 或 OLED_FONT_8X16
  */
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t font);

/**
  * @brief  显示字符串
  * @param  x      列起始坐标
  * @param  y      页坐标
  * @param  str    要显示的字符串（以 '\0' 结尾）
  * @param  font   字体：OLED_FONT_6X8 或 OLED_FONT_8X16
  */
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t font);

/**
  * @brief  显示无符号整数（十进制）
  * @param  x      列起始坐标
  * @param  y      页坐标
  * @param  num    要显示的数字
  * @param  len    显示位数（不足前补零，0 表示不补零）
  * @param  font   字体
  */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t font);

/**
  * @brief  显示浮点数
  * @param  x       列起始坐标
  * @param  y       页坐标
  * @param  num     要显示的数字
  * @param  intLen  整数部分位数
  * @param  decLen  小数部分位数
  * @param  font    字体
  */
void OLED_ShowFloat(uint8_t x, uint8_t y, double num, uint8_t intLen, uint8_t decLen, uint8_t font);

/**
  * @brief  显示中文（16×16 点阵）
  * @param  x       列起始坐标
  * @param  y       页坐标
  * @param  index   汉字在字库表中的索引（用户需自定义字库数组）
  */
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t index);

/**
  * @brief  画直线（Bresenham 算法）
  * @param  x1, y1  起点
  * @param  x2, y2  终点
  * @param  color   0 熄灭，1 点亮
  */
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

/**
  * @brief  画矩形框
  * @param  x1, y1  左上角
  * @param  x2, y2  右下角
  * @param  color   0 熄灭，1 点亮
  */
void OLED_DrawRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

/**
  * @brief  画圆（Bresenham 算法）
  * @param  xc, yc  圆心
  * @param  r       半径
  * @param  color   0 熄灭，1 点亮
  */
void OLED_DrawCircle(uint8_t xc, uint8_t yc, uint8_t r, uint8_t color);

#endif
