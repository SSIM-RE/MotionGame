#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>

// 颜色定义
#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_YELLOW      0xFFE0

// 初始化
void Display_Init(void);

// 基本绘图函数
void Display_FillRect(int x, int y, int w, int h, uint16_t color);
void Display_DrawBitmap1B(int x, int y, int w, int h, const uint8_t* bitmap,
                         uint16_t fg_color, uint16_t bg_color);

// 双缓冲相关函数
void Display_StartFrame(void);
void Display_EndFrame(void);
void Display_SetDoubleBufferEnabled(bool enabled);

// 清屏
void Display_ClearScreen(uint16_t color);

#endif // DISPLAY_DRIVER_H