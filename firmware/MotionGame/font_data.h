#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <stdint.h>
#include <stdbool.h>

// 字体ID - 这里只定义字体数据ID，显示字体ID在display_driver.h中定义
typedef enum {
    FONT_DATA_5x7 = 0,      // 5x7 像素字体数据
    FONT_DATA_7x10,         // 7x10 像素字体数据
    FONT_DATA_COUNT
} FontDataID_t;

// 字体信息结构
typedef struct {
    const uint8_t* data;      // 字体位图数据
    uint8_t width;            // 字符宽度
    uint8_t height;           // 字符高度
    uint8_t first_char;       // 第一个ASCII字符
    uint8_t last_char;        // 最后一个ASCII字符
    uint8_t char_spacing;     // 字符间距
    const char* name;         // 字体名称
} FontInfo_t;

// 获取字体信息
const FontInfo_t* Font_GetInfo(FontDataID_t font_id);

// 检查字符是否在字体中
bool Font_CharInRange(FontDataID_t font_id, char ch);

// 获取字符位图
const uint8_t* Font_GetCharBitmap(FontDataID_t font_id, char ch, 
                                  uint8_t* width, uint8_t* height);

// 获取字符串宽度
int Font_GetStringWidth(FontDataID_t font_id, const char* str, uint8_t size);

// 获取字符串高度
int Font_GetStringHeight(FontDataID_t font_id, uint8_t size);

#endif // FONT_DATA_H