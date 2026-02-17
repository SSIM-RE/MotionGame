// display_driver.cpp - 增强版显示驱动
#include "display_driver.h"
#include <TFT_eSPI.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "font_data.h"
#include <Arduino.h>

// 添加swap函数
template <typename T>
static void swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

// ==================== 全局变量 ====================
static TFT_eSPI tft;                                // TFT驱动实例
static uint16_t frame_buffer[128 * 160];           // 双缓冲帧缓冲区
static bool double_buffer_enabled = true;          // 双缓冲启用标志

// 文本参数
static TextParams_t text_params = {
    .font = DISPLAY_FONT_5x7,
    .color = TFT_WHITE,
    .bg_color = TFT_BLACK,
    .size = 1,
    .h_align = TEXT_ALIGN_LEFT,
    .v_align = TEXT_ALIGN_TOP,
    .transparent = false
};

// ==================== 私有辅助函数 ====================

// 边界检查函数
static bool checkBounds(int x, int y) {
    return (x >= 0 && x < 128 && y >= 0 && y < 160);
}

static bool checkRectBounds(int x, int y, int w, int h) {
    return (x < 128 && y < 160 && x + w > 0 && y + h > 0);
}

// 计算裁剪后的矩形
static void clipRect(int* x, int* y, int* w, int* h) {
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > 128) *w = 128 - *x;
    if (*y + *h > 160) *h = 160 - *y;
}

// 颜色混合函数
static uint16_t blendColors(uint16_t color1, uint16_t color2, float alpha) {
    if (alpha <= 0.0f) return color1;
    if (alpha >= 1.0f) return color2;
    
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    
    uint8_t r = (uint8_t)(r1 + (r2 - r1) * alpha);
    uint8_t g = (uint8_t)(g1 + (g2 - g1) * alpha);
    uint8_t b = (uint8_t)(b1 + (b2 - b1) * alpha);
    
    return (r << 11) | (g << 5) | b;
}

// ==================== 初始化函数 ====================

void Display_Init(void) {
    SPI.begin(2, -1, 3, 7);
    tft.init();
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
    
    // 初始化双缓冲
    Display_ClearBuffer(TFT_BLACK);
    Display_EndFrame();  // 立即显示
    
    Serial.println("Display initialized (128x160, Double Buffer)");
}

// ==================== 双缓冲控制函数 ====================

void Display_SetDoubleBufferEnabled(bool enabled) {
    double_buffer_enabled = enabled;
    Serial.printf("Double buffer %s\n", enabled ? "enabled" : "disabled");
}

bool Display_GetDoubleBufferEnabled(void) {
    return double_buffer_enabled;
}

void Display_StartFrame(void) {
    // 开始新的一帧
}

void Display_EndFrame(void) {
    if (double_buffer_enabled) {
        tft.startWrite();
        tft.setAddrWindow(0, 0, 128, 160);
        tft.pushColors(frame_buffer, 128 * 160, true);
        tft.endWrite();
    }
}

void Display_ClearScreen(uint16_t color) {
    if (double_buffer_enabled) {
        Display_ClearBuffer(color);
        Display_EndFrame();
    } else {
        tft.fillScreen(color);
    }
}

void Display_ClearBuffer(uint16_t color) {
    if (double_buffer_enabled) {
        for (int i = 0; i < 128 * 160; i++) {
            frame_buffer[i] = color;
        }
    } else {
        tft.fillScreen(color);
    }
}

// ==================== 高级绘图函数 ====================

void Display_DrawPixelAlpha(int x, int y, uint16_t color, float alpha) {
    if (!checkBounds(x, y)) return;
    
    if (alpha >= 1.0f) {
        Display_DrawPixel(x, y, color);
    } else if (alpha > 0.0f) {
        if (double_buffer_enabled) {
            uint16_t bg_color = frame_buffer[y * 128 + x];
            uint16_t blended = blendColors(bg_color, color, alpha);
            frame_buffer[y * 128 + x] = blended;
        } else {
            // 直接模式下无法混合
            Display_DrawPixel(x, y, color);
        }
    }
}

void Display_DrawLineDashed(int x0, int y0, int x1, int y1, uint16_t color, uint8_t pattern) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int step = 0;
    
    while (true) {
        if ((pattern >> (step % 8)) & 0x01) {
            Display_DrawPixel(x0, y0, color);
        }
        step++;
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Display_DrawRoundRect(int x, int y, int w, int h, int radius, uint16_t color) {
    if (radius < 0) radius = 0;
    if (radius > w/2) radius = w/2;
    if (radius > h/2) radius = h/2;
    
    // 绘制四角圆弧
    Display_DrawCircleQuarter(x + radius, y + radius, radius, 0b1000, color); // 左上
    Display_DrawCircleQuarter(x + w - radius, y + radius, radius, 0b0100, color); // 右上
    Display_DrawCircleQuarter(x + w - radius, y + h - radius, radius, 0b0010, color); // 右下
    Display_DrawCircleQuarter(x + radius, y + h - radius, radius, 0b0001, color); // 左下
    
    // 绘制直线部分
    Display_DrawLine(x + radius, y, x + w - radius, y, color); // 上边
    Display_DrawLine(x + radius, y + h - 1, x + w - radius, y + h - 1, color); // 下边
    Display_DrawLine(x, y + radius, x, y + h - radius, color); // 左边
    Display_DrawLine(x + w - 1, y + radius, x + w - 1, y + h - radius, color); // 右边
}

void Display_FillRoundRect(int x, int y, int w, int h, int radius, uint16_t color) {
    if (radius < 0) radius = 0;
    if (radius > w/2) radius = w/2;
    if (radius > h/2) radius = h/2;
    
    // 填充中间矩形
    Display_FillRect(x + radius, y, w - 2 * radius, h, color);
    
    // 填充四个边的矩形
    Display_FillRect(x, y + radius, radius, h - 2 * radius, color);
    Display_FillRect(x + w - radius, y + radius, radius, h - 2 * radius, color);
    
    // 填充四个角的圆角
    Display_FillCircleQuarter(x + radius, y + radius, radius, 0b1000, color);
    Display_FillCircleQuarter(x + w - radius, y + radius, radius, 0b0100, color);
    Display_FillCircleQuarter(x + w - radius, y + h - radius, radius, 0b0010, color);
    Display_FillCircleQuarter(x + radius, y + h - radius, radius, 0b0001, color);
}

// 绘制四分之一圆
void Display_DrawCircleQuarter(int x, int y, int radius, uint8_t quarters, uint16_t color) {
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int px = 0;
    int py = radius;
    
    while (px < py) {
        if (f >= 0) {
            py--;
            ddF_y += 2;
            f += ddF_y;
        }
        px++;
        ddF_x += 2;
        f += ddF_x;
        
        // 根据quarters绘制对应象限的点
        if (quarters & 0b1000) { // 左上
            Display_DrawPixel(x - py, y - px, color);
            Display_DrawPixel(x - px, y - py, color);
        }
        if (quarters & 0b0100) { // 右上
            Display_DrawPixel(x + px, y - py, color);
            Display_DrawPixel(x + py, y - px, color);
        }
        if (quarters & 0b0010) { // 右下
            Display_DrawPixel(x + py, y + px, color);
            Display_DrawPixel(x + px, y + py, color);
        }
        if (quarters & 0b0001) { // 左下
            Display_DrawPixel(x - px, y + py, color);
            Display_DrawPixel(x - py, y + px, color);
        }
    }
}

// 填充四分之一圆
void Display_FillCircleQuarter(int x, int y, int radius, uint8_t quarters, uint16_t color) {
    for (int py = -radius; py <= radius; py++) {
        for (int px = -radius; px <= radius; px++) {
            if (px*px + py*py <= radius*radius) {
                bool draw = false;
                
                if (quarters & 0b1000 && px <= 0 && py <= 0) draw = true; // 左上
                if (quarters & 0b0100 && px >= 0 && py <= 0) draw = true; // 右上
                if (quarters & 0b0010 && px >= 0 && py >= 0) draw = true; // 右下
                if (quarters & 0b0001 && px <= 0 && py >= 0) draw = true; // 左下
                
                if (draw) {
                    Display_DrawPixel(x + px, y + py, color);
                }
            }
        }
    }
}

// ==================== 基本绘图函数（双缓冲） ====================

void Display_DrawPixel(int x, int y, uint16_t color) {
    if (!checkBounds(x, y)) return;
    
    if (double_buffer_enabled) {
        frame_buffer[y * 128 + x] = color;
    } else {
        tft.drawPixel(x, y, color);
    }
}

uint16_t Display_GetPixel(int x, int y) {
    if (!checkBounds(x, y)) return 0;
    
    if (double_buffer_enabled) {
        return frame_buffer[y * 128 + x];
    } else {
        // 注意：直接模式可能无法获取像素
        return 0;
    }
}

void Display_DrawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    // Bresenham直线算法
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        Display_DrawPixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Display_DrawRect(int x, int y, int w, int h, uint16_t color) {
    Display_DrawLine(x, y, x + w - 1, y, color);
    Display_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    Display_DrawLine(x, y, x, y + h - 1, color);
    Display_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void Display_FillRect(int x, int y, int w, int h, uint16_t color) {
    if (!checkRectBounds(x, y, w, h)) return;
    
    clipRect(&x, &y, &w, &h);
    if (w <= 0 || h <= 0) return;
    
    if (double_buffer_enabled) {
        for (int py = 0; py < h; py++) {
            int buffer_y = y + py;
            int buffer_index = buffer_y * 128 + x;
            
            for (int px = 0; px < w; px++) {
                frame_buffer[buffer_index + px] = color;
            }
        }
    } else {
        tft.fillRect(x, y, w, h, color);
    }
}

void Display_DrawCircle(int x, int y, int radius, uint16_t color) {
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int px = 0;
    int py = radius;
    
    Display_DrawPixel(x, y + radius, color);
    Display_DrawPixel(x, y - radius, color);
    Display_DrawPixel(x + radius, y, color);
    Display_DrawPixel(x - radius, y, color);
    
    while (px < py) {
        if (f >= 0) {
            py--;
            ddF_y += 2;
            f += ddF_y;
        }
        px++;
        ddF_x += 2;
        f += ddF_x;
        
        Display_DrawPixel(x + px, y + py, color);
        Display_DrawPixel(x - px, y + py, color);
        Display_DrawPixel(x + px, y - py, color);
        Display_DrawPixel(x - px, y - py, color);
        Display_DrawPixel(x + py, y + px, color);
        Display_DrawPixel(x - py, y + px, color);
        Display_DrawPixel(x + py, y - px, color);
        Display_DrawPixel(x - py, y - px, color);
    }
}

void Display_FillCircle(int x, int y, int radius, uint16_t color) {
    for (int py = -radius; py <= radius; py++) {
        int dx = (int)sqrt(radius * radius - py * py);
        Display_DrawLine(x - dx, y + py, x + dx, y + py, color);
    }
}

void Display_DrawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
    Display_DrawLine(x0, y0, x1, y1, color);
    Display_DrawLine(x1, y1, x2, y2, color);
    Display_DrawLine(x2, y2, x0, y0, color);
}

void Display_FillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) {
    int16_t a, b, y, last;
    int16_t dx01, dy01, dx02, dy02, dx12, dy12;
    int32_t sa = 0, sb = 0;
    
    // 对y坐标排序
    if (y0 > y1) { ::swap(y0, y1); ::swap(x0, x1); }
    if (y1 > y2) { ::swap(y2, y1); ::swap(x2, x1); }
    if (y0 > y1) { ::swap(y0, y1); ::swap(x0, x1); }
    
    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1;
        else if (x1 > b) b = x1;
        if (x2 < a) a = x2;
        else if (x2 > b) b = x2;
        Display_DrawLine(a, y0, b, y0, color);
        return;
    }
    
    dx01 = x1 - x0;
    dy01 = y1 - y0;
    dx02 = x2 - x0;
    dy02 = y2 - y0;
    dx12 = x2 - x1;
    dy12 = y2 - y1;
    
    last = y1 == y2 ? y1 : y1 - 1;
    
    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        
        if (a > b) ::swap(a, b);
        Display_DrawLine(a, y, b, y, color);
    }
    
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        
        if (a > b) ::swap(a, b);
        Display_DrawLine(a, y, b, y, color);
    }
}

// ==================== 位图绘制函数 ====================

void Display_DrawBitmap1B(int x, int y, int w, int h, const uint8_t* bitmap,
                         uint16_t fg_color, uint16_t bg_color) {
    if (!checkRectBounds(x, y, w, h)) return;
    
    int draw_x = x;
    int draw_y = y;
    int draw_w = w;
    int draw_h = h;
    
    if (draw_x < 0) { draw_w += draw_x; draw_x = 0; }
    if (draw_y < 0) { draw_h += draw_y; draw_y = 0; }
    if (draw_x + draw_w > 128) draw_w = 128 - draw_x;
    if (draw_y + draw_h > 160) draw_h = 160 - draw_y;
    
    if (draw_w <= 0 || draw_h <= 0) return;
    
    int bytes_per_row = (w + 7) / 8;
    
    for (int py = 0; py < draw_h; py++) {
        int src_y = py + (y < 0 ? -y : 0);
        
        for (int px = 0; px < draw_w; px++) {
            int src_x = px + (x < 0 ? -x : 0);
            
            int byte_index = src_y * bytes_per_row + (src_x / 8);
            int bit_index = 7 - (src_x % 8);
            
            if (byte_index >= 0 && byte_index < bytes_per_row * h) {
                uint8_t pixel_byte = bitmap[byte_index];
                bool pixel_value = (pixel_byte >> bit_index) & 0x01;
                
                uint16_t color = pixel_value ? fg_color : bg_color;
                Display_DrawPixel(draw_x + px, draw_y + py, color);
            }
        }
    }
}

void Display_DrawBitmapRGB(int x, int y, int w, int h, const uint16_t* bitmap) {
    if (!checkRectBounds(x, y, w, h)) return;
    
    int draw_x = x;
    int draw_y = y;
    int draw_w = w;
    int draw_h = h;
    
    if (draw_x < 0) { draw_w += draw_x; draw_x = 0; }
    if (draw_y < 0) { draw_h += draw_y; draw_y = 0; }
    if (draw_x + draw_w > 128) draw_w = 128 - draw_x;
    if (draw_y + draw_h > 160) draw_h = 160 - draw_y;
    
    if (draw_w <= 0 || draw_h <= 0) return;
    
    for (int py = 0; py < draw_h; py++) {
        int src_y = py + (y < 0 ? -y : 0);
        
        for (int px = 0; px < draw_w; px++) {
            int src_x = px + (x < 0 ? -x : 0);
            
            int src_index = src_y * w + src_x;
            if (src_index >= 0 && src_index < w * h) {
                Display_DrawPixel(draw_x + px, draw_y + py, bitmap[src_index]);
            }
        }
    }
}

// ==================== 文本绘制函数 ====================

// 将DisplayFont_t映射到FontDataID_t
static FontDataID_t mapDisplayFontToDataFont(DisplayFont_t display_font) {
    switch (display_font) {
        case DISPLAY_FONT_5x7:
            return FONT_DATA_5x7;
        case DISPLAY_FONT_7x10:
            return FONT_DATA_7x10;
        default:
            return FONT_DATA_5x7;
    }
}

// 获取有效参数
static TextParams_t getEffectiveParams(const TextParams_t* params) {
    if (params) {
        return *params;
    } else {
        return text_params;
    }
}

// 绘制一个字符到双缓冲
static void drawCharToBuffer(int x, int y, char ch, const TextParams_t* params) {
    if (!checkBounds(x, y) || ch < 32) return;
    
    FontDataID_t data_font = mapDisplayFontToDataFont(params->font);
    uint8_t char_width, char_height;
    const uint8_t* char_bitmap = Font_GetCharBitmap(data_font, ch, &char_width, &char_height);
    
    if (!char_bitmap) return;
    
    // 绘制背景
    if (!params->transparent) {
        Display_FillRect(x, y, 
                        char_width * params->size, 
                        char_height * params->size, 
                        params->bg_color);
    }
    
    // 5x7字体：5字节，每字节一列，bit0在顶部
    if (data_font == FONT_DATA_5x7) {
        for (int col = 0; col < char_width; col++) {
            uint8_t column_data = char_bitmap[col];
            
            for (int row = 0; row < char_height; row++) {
                bool pixel_on = (column_data >> row) & 0x01;
                
                if (pixel_on) {
                    if (params->size == 1) {
                        int pixel_x = x + col;
                        int pixel_y = y + row;
                        if (checkBounds(pixel_x, pixel_y)) {
                            Display_DrawPixel(pixel_x, pixel_y, params->color);
                        }
                    } else {
                        for (int sy = 0; sy < params->size; sy++) {
                            for (int sx = 0; sx < params->size; sx++) {
                                int pixel_x = x + col * params->size + sx;
                                int pixel_y = y + row * params->size + sy;
                                
                                if (checkBounds(pixel_x, pixel_y)) {
                                    Display_DrawPixel(pixel_x, pixel_y, params->color);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        // 其他字体
        int bytes_per_row = (char_width + 7) / 8;
        
        for (int row = 0; row < char_height; row++) {
            for (int col = 0; col < char_width; col++) {
                int byte_index = row * bytes_per_row + (col / 8);
                int bit_index = 7 - (col % 8);
                
                uint8_t pixel_byte = char_bitmap[byte_index];
                bool pixel_on = (pixel_byte >> bit_index) & 0x01;
                
                if (pixel_on) {
                    if (params->size == 1) {
                        int pixel_x = x + col;
                        int pixel_y = y + row;
                        if (checkBounds(pixel_x, pixel_y)) {
                            Display_DrawPixel(pixel_x, pixel_y, params->color);
                        }
                    } else {
                        for (int sy = 0; sy < params->size; sy++) {
                            for (int sx = 0; sx < params->size; sx++) {
                                int pixel_x = x + col * params->size + sx;
                                int pixel_y = y + row * params->size + sy;
                                
                                if (checkBounds(pixel_x, pixel_y)) {
                                    Display_DrawPixel(pixel_x, pixel_y, params->color);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// 绘制字符串到双缓冲
static void drawStringToBuffer(int x, int y, const char* str, const TextParams_t* params) {
    if (!str || !*str) return;
    
    FontDataID_t data_font = mapDisplayFontToDataFont(params->font);
    const FontInfo_t* font_info = Font_GetInfo(data_font);
    if (!font_info) return;
    
    int cursor_x = x;
    int cursor_y = y;
    int line_height = font_info->height * params->size + 2;
    
    while (*str) {
        char ch = *str;
        
        if (ch == '\n') {
            cursor_x = x;
            cursor_y += line_height;
            str++;
            continue;
        }
        
        uint8_t char_width, char_height;
        Font_GetCharBitmap(data_font, ch, &char_width, &char_height);
        int char_draw_width = char_width * params->size;
        
        if (cursor_x + char_draw_width > 128 && cursor_x != x) {
            cursor_x = x;
            cursor_y += line_height;
        }
        
        drawCharToBuffer(cursor_x, cursor_y, ch, params);
        
        cursor_x += char_draw_width + font_info->char_spacing;
        str++;
    }
}

// 计算文本位置
static void calculateTextPosition(int* x, int* y, const char* str, const TextParams_t* params) {
    int width = Display_GetStringWidth(str, params->font, params->size);
    int height = Display_GetStringHeight(str, params->font, params->size);
    
    switch (params->h_align) {
        case TEXT_ALIGN_CENTER:
            *x -= width / 2;
            break;
        case TEXT_ALIGN_RIGHT:
            *x -= width;
            break;
        case TEXT_ALIGN_LEFT:
        default:
            break;
    }
    
    switch (params->v_align) {
        case TEXT_ALIGN_MIDDLE:
            *y -= height / 2;
            break;
        case TEXT_ALIGN_BOTTOM:
            *y -= height;
            break;
        case TEXT_ALIGN_TOP:
        default:
            break;
    }
}

// ==================== 公共文本函数实现 ====================

void Display_SetTextParams(const TextParams_t* params) {
    if (params) {
        text_params = *params;
    }
}

TextParams_t Display_GetTextParams(void) {
    return text_params;
}

void Display_SetFont(DisplayFont_t font) {
    text_params.font = font;
}

void Display_SetTextColor(uint16_t color, uint16_t bg_color) {
    text_params.color = color;
    text_params.bg_color = bg_color;
}

void Display_SetTextSize(uint8_t size) {
    text_params.size = size;
}

void Display_DrawChar(int x, int y, char ch, const TextParams_t* params) {
    TextParams_t effective_params = getEffectiveParams(params);
    
    if (double_buffer_enabled) {
        drawCharToBuffer(x, y, ch, &effective_params);
    } else {
        tft.setTextFont(1);
        tft.setTextSize(effective_params.size);
        tft.setTextColor(effective_params.color, effective_params.bg_color);
        tft.setCursor(x, y);
        tft.print(ch);
    }
}

void Display_DrawString(int x, int y, const char* str, const TextParams_t* params) {
    if (!str || !*str) return;
    
    TextParams_t effective_params = getEffectiveParams(params);
    
    int actual_x = x;
    int actual_y = y;
    calculateTextPosition(&actual_x, &actual_y, str, &effective_params);
    
    if (double_buffer_enabled) {
        drawStringToBuffer(actual_x, actual_y, str, &effective_params);
    } else {
        tft.setTextFont(1);
        tft.setTextSize(effective_params.size);
        tft.setTextColor(effective_params.color, effective_params.bg_color);
        tft.setCursor(actual_x, actual_y);
        tft.print(str);
    }
}

void Display_DrawStringF(int x, int y, const TextParams_t* params, const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Display_DrawString(x, y, buffer, params);
}

void Display_DrawText(int x, int y, const char* str) {
    Display_DrawString(x, y, str, NULL);
}

void Display_DrawTextF(int x, int y, const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Display_DrawString(x, y, buffer, NULL);
}

void Display_DrawTextCentered(int center_x, int center_y, const char* str, const TextParams_t* params) {
    TextParams_t effective_params = getEffectiveParams(params);
    effective_params.h_align = TEXT_ALIGN_CENTER;
    effective_params.v_align = TEXT_ALIGN_MIDDLE;
    
    Display_DrawString(center_x, center_y, str, &effective_params);
}

void Display_DrawTextCenteredSimple(int center_x, int center_y, const char* str) {
    Display_DrawTextCentered(center_x, center_y, str, NULL);
}

void Display_DrawTextInRect(int x, int y, int w, int h, const char* str, const TextParams_t* params) {
    TextParams_t effective_params = getEffectiveParams(params);
    effective_params.h_align = TEXT_ALIGN_CENTER;
    effective_params.v_align = TEXT_ALIGN_MIDDLE;
    
    Display_DrawString(x + w/2, y + h/2, str, &effective_params);
}

void Display_DrawInt(int x, int y, int value, const TextParams_t* params) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", value);
    Display_DrawString(x, y, buffer, params);
}

void Display_DrawFloat(int x, int y, float value, int decimals, const TextParams_t* params) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    Display_DrawString(x, y, buffer, params);
}

// 文本测量函数
int Display_GetStringWidth(const char* str, DisplayFont_t font, uint8_t size) {
    FontDataID_t data_font = mapDisplayFontToDataFont(font);
    return Font_GetStringWidth(data_font, str, size);
}

int Display_GetStringHeight(const char* str, DisplayFont_t font, uint8_t size) {
    FontDataID_t data_font = mapDisplayFontToDataFont(font);
    return Font_GetStringHeight(data_font, size);
}

void Display_GetStringBounds(const char* str, DisplayFont_t font, uint8_t size, 
                            int* width, int* height) {
    if (width) *width = Display_GetStringWidth(str, font, size);
    if (height) *height = Display_GetStringHeight(str, font, size);
}

int Display_GetCharWidth(char ch, DisplayFont_t font, uint8_t size) {
    FontDataID_t data_font = mapDisplayFontToDataFont(font);
    const FontInfo_t* info = Font_GetInfo(data_font);
    if (!info) return 0;
    
    return info->width * size;
}

// 高级功能
void Display_DrawTextWithShadow(int x, int y, const char* str, 
                               uint16_t text_color, uint16_t shadow_color,
                               const TextParams_t* base_params) {
    TextParams_t shadow_params = getEffectiveParams(base_params);
    shadow_params.color = shadow_color;
    
    TextParams_t text_params = getEffectiveParams(base_params);
    text_params.color = text_color;
    
    Display_DrawString(x + 1, y + 1, str, &shadow_params);
    Display_DrawString(x, y, str, &text_params);
}

void Display_DrawTextGradient(int x, int y, const char* str,
                             uint16_t start_color, uint16_t end_color,
                             const TextParams_t* base_params) {
    // 注意：这个功能需要更复杂的实现
    // 这里简化处理，直接绘制
    TextParams_t params = getEffectiveParams(base_params);
    params.color = start_color;
    Display_DrawString(x, y, str, &params);
}

// 调试函数
void Display_TestFontFormat() {
    Serial.println("\n=== Testing Font Format ===");
    
    FontDataID_t font_id = FONT_DATA_5x7;
    
    uint8_t width, height;
    const uint8_t* bitmap = Font_GetCharBitmap(font_id, 'A', &width, &height);
    
    if (!bitmap) {
        Serial.println("ERROR: No bitmap for 'A'");
        return;
    }
    
    Serial.printf("Font 5x7 - Char 'A': width=%d, height=%d\n", width, height);
    Serial.print("Raw data: ");
    for (int i = 0; i < 5; i++) {
        Serial.printf("%02X ", bitmap[i]);
    }
    Serial.println();
    
    Serial.println("\nPreview (bit0 at bottom):");
    for (int row = 0; row < 7; row++) {
        Serial.print("  ");
        for (int col = 0; col < 5; col++) {
            uint8_t data = bitmap[col];
            bool pixel = (data >> row) & 0x01;
            Serial.print(pixel ? "##" : "  ");
        }
        Serial.println();
    }
}

void Display_DebugFont(DisplayFont_t font, char ch) {
    FontDataID_t data_font = mapDisplayFontToDataFont(font);
    uint8_t width, height;
    const uint8_t* bitmap = Font_GetCharBitmap(data_font, ch, &width, &height);
    
    Serial.printf("Font debug - Char '%c' (%d):\n", ch, ch);
    Serial.printf("  Mapped font: %d -> %d\n", font, data_font);
    Serial.printf("  Size: %dx%d\n", width, height);
    
    if (!bitmap) {
        Serial.println("  NO BITMAP!");
        return;
    }
    
    Serial.printf("  Bitmap data: ");
    for (int i = 0; i < 5; i++) {
        Serial.printf("%02X ", bitmap[i]);
    }
    Serial.println();
    
    Serial.println("  Preview:");
    for (int row = 0; row < 7; row++) {
        Serial.print("    ");
        for (int col = 0; col < 5; col++) {
            uint8_t column_data = bitmap[col];
            bool pixel = (column_data >> row) & 0x01;
            Serial.print(pixel ? "##" : "  ");
        }
        Serial.println();
    }
}