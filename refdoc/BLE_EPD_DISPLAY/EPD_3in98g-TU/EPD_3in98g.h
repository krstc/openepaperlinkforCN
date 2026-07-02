#ifndef __EPD_3IN98G_H_
#define __EPD_3IN98G_H_

#include "DEV_Config.h"

// 显示分辨率 - 请根据你的屏幕数据手册确认
#define EPD_3IN98G_WIDTH       552
#define EPD_3IN98G_HEIGHT      768

// 颜色定义
#define EPD_3IN98G_BLACK   0x0
#define EPD_3IN98G_WHITE   0x1
#define EPD_3IN98G_YELLOW  0x2
#define EPD_3IN98G_RED     0x3

// 函数声明
void EPD_3IN98G_Init(void);
void EPD_3IN98G_Clear(UBYTE color);
void EPD_3IN98G_Display(const UBYTE *Image);
void EPD_3IN98G_Sleep(void);

#endif