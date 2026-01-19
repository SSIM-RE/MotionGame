#include "display_driver.h"
#include <TFT_eSPI.h>

static TFT_eSPI tft;

// 双缓冲相关
static uint16_t frame_buffer[128 * 160];  // 静态分配，避免动态内存问题
static bool double_buffer_enabled = true;

void Display_Init(void)
{   SPI.begin(2,-1,3,7);
    tft.init();
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
}

void Display_SetDoubleBufferEnabled(bool enabled)
{
    double_buffer_enabled = enabled;
}

void Display_StartFrame(void)
{
    // 双缓冲准备，无需特别操作
}

void Display_EndFrame(void)
{
    if (double_buffer_enabled)
    {
        // 将缓冲区内容写入显示
        tft.startWrite();
        tft.setAddrWindow(0, 0, 128, 160);
        tft.pushColors(frame_buffer, 128 * 160, true);
        tft.endWrite();
    }
}

void Display_FillRect(int x, int y, int w, int h, uint16_t color)
{
    if (double_buffer_enabled)
    {
        // 边界检查
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > 128) w = 128 - x;
        if (y + h > 160) h = 160 - y;
        
        if (w <= 0 || h <= 0) return;
        
        // 在缓冲区中填充
        for (int py = 0; py < h; py++)
        {
            for (int px = 0; px < w; px++)
            {
                int buffer_x = x + px;
                int buffer_y = y + py;
                
                if (buffer_x >= 0 && buffer_x < 128 &&
                    buffer_y >= 0 && buffer_y < 160)
                {
                    int buffer_index = buffer_y * 128 + buffer_x;
                    frame_buffer[buffer_index] = color;
                }
            }
        }
    }
    else
    {
        // 直接绘制
        tft.fillRect(x, y, w, h, color);
    }
}

void Display_DrawBitmap1B(int x, int y, int w, int h, const uint8_t* bitmap,
                         uint16_t fg_color, uint16_t bg_color)
{
    if (double_buffer_enabled)
    {
        // 边界检查
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > 128) w = 128 - x;
        if (y + h > 160) h = 160 - y;
        
        if (w <= 0 || h <= 0) return;
        
        int bytes_per_row = (w + 7) / 8;
        
        for (int py = 0; py < h; py++)
        {
            for (int px = 0; px < w; px++)
            {
                int byte_index = py * bytes_per_row + (px / 8);
                int bit_index = 7 - (px % 8);
                
                uint8_t pixel_byte = bitmap[byte_index];
                bool pixel_value = (pixel_byte >> bit_index) & 0x01;
                
                uint16_t color = pixel_value ? fg_color : bg_color;
                
                int buffer_index = (y + py) * 128 + (x + px);
                if (buffer_index >= 0 && buffer_index < 128 * 160)
                {
                    frame_buffer[buffer_index] = color;
                }
            }
        }
    }
    else
    {
        // 直接绘制
        tft.startWrite();
        for (int py = 0; py < h; py++)
        {
            for (int px = 0; px < w; px++)
            {
                int byte_index = py * ((w + 7) / 8) + (px / 8);
                int bit_index = 7 - (px % 8);
                
                uint8_t pixel_byte = bitmap[byte_index];
                bool pixel_value = (pixel_byte >> bit_index) & 0x01;
                
                uint16_t color = pixel_value ? fg_color : bg_color;
                tft.drawPixel(x + px, y + py, color);
            }
        }
        tft.endWrite();
    }
}

void Display_ClearScreen(uint16_t color)
{
    if (double_buffer_enabled)
    {
        // 清空缓冲区
        for (int i = 0; i < 128 * 160; i++)
        {
            frame_buffer[i] = color;
        }
        // 立即显示
        Display_EndFrame();
    }
    else
    {
        tft.fillScreen(color);
    }
}