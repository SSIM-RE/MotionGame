#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// ==================== 颜色定义 ====================
#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_YELLOW      0xFFE0
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_ORANGE      0xFC00
#define TFT_DARKGRAY    0x2945
#define TFT_LIGHTGRAY   0xCE79

// ==================== 字体定义 ====================
typedef enum {
    DISPLAY_FONT_5x7 = 0,   // 5x7 像素字体
    DISPLAY_FONT_7x10,      // 7x10 像素字体
    DISPLAY_FONT_COUNT
} DisplayFont_t;

// ==================== 文本对齐方式 ====================
typedef enum {
    TEXT_ALIGN_LEFT = 0,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_TOP,
    TEXT_ALIGN_MIDDLE,
    TEXT_ALIGN_BOTTOM
} TextAlign_t;

// ==================== 文本渲染参数 ====================
typedef struct {
    DisplayFont_t font;     // 字体类型
    uint16_t color;         // 文本颜色
    uint16_t bg_color;      // 背景颜色
    uint8_t size;           // 字体大小（缩放倍数）
    TextAlign_t h_align;    // 水平对齐
    TextAlign_t v_align;    // 垂直对齐
    bool transparent;       // 是否透明背景
} TextParams_t;

// ==================== 函数声明 ====================

// -------------------- 初始化 --------------------
void Display_Init(void);

// -------------------- 双缓冲控制 --------------------
void Display_StartFrame(void);
void Display_EndFrame(void);
void Display_SetDoubleBufferEnabled(bool enabled);
bool Display_GetDoubleBufferEnabled(void);
void Display_ClearScreen(uint16_t color);
void Display_ClearBuffer(uint16_t color);

// -------------------- 基本绘图函数 --------------------
void Display_DrawPixel(int x, int y, uint16_t color);
void Display_DrawLine(int x0, int y0, int x1, int y1, uint16_t color);
void Display_DrawRect(int x, int y, int w, int h, uint16_t color);
void Display_FillRect(int x, int y, int w, int h, uint16_t color);
void Display_DrawCircle(int x, int y, int radius, uint16_t color);
void Display_FillCircle(int x, int y, int radius, uint16_t color);
void Display_DrawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void Display_FillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);

// -------------------- 位图绘制 --------------------
void Display_DrawBitmap1B(int x, int y, int w, int h, const uint8_t* bitmap,
                         uint16_t fg_color, uint16_t bg_color);
void Display_DrawBitmapRGB(int x, int y, int w, int h, const uint16_t* bitmap);

// -------------------- 文本绘制（完全双缓冲） --------------------
// 设置文本参数
void Display_SetTextParams(const TextParams_t* params);
TextParams_t Display_GetTextParams(void);
void Display_SetFont(DisplayFont_t font);
void Display_SetTextColor(uint16_t color, uint16_t bg_color);
void Display_SetTextSize(uint8_t size);

// 基础文本绘制
void Display_DrawChar(int x, int y, char ch, const TextParams_t* params);
void Display_DrawString(int x, int y, const char* str, const TextParams_t* params);
void Display_DrawStringF(int x, int y, const TextParams_t* params, const char* format, ...);

// 快捷文本绘制（使用当前参数）
void Display_DrawText(int x, int y, const char* str);
void Display_DrawTextF(int x, int y, const char* format, ...);

// 特殊对齐文本绘制
void Display_DrawTextCentered(int center_x, int center_y, const char* str, const TextParams_t* params);
void Display_DrawTextCenteredSimple(int center_x, int center_y, const char* str);
void Display_DrawTextInRect(int x, int y, int w, int h, const char* str, const TextParams_t* params);

// 数字绘制
void Display_DrawInt(int x, int y, int value, const TextParams_t* params);
void Display_DrawFloat(int x, int y, float value, int decimals, const TextParams_t* params);

// 文本测量
int Display_GetStringWidth(const char* str, DisplayFont_t font, uint8_t size);
int Display_GetStringHeight(const char* str, DisplayFont_t font, uint8_t size);
void Display_GetStringBounds(const char* str, DisplayFont_t font, uint8_t size, 
                            int* width, int* height);
int Display_GetCharWidth(char ch, DisplayFont_t font, uint8_t size);
void Display_TestFontFormat();
#endif // DISPLAY_DRIVER_H