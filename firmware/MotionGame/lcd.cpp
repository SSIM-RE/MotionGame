#include "lcd.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

/* ===== 硬件引脚 ===== */
#define TFT_CS   7
#define TFT_RST  10
#define TFT_DC   6
#define TFT_MOSI 3
#define TFT_SCLK 2

/* ===== 颜色定义 ===== */
#define LCD_BLACK   0x0000
#define LCD_YELLOW  0x07FF
#define LCD_TEXT    LCD_BLACK
#define LCD_BG      LCD_YELLOW
#define Display_Color_Black    0x0000    //黑色
#define Display_Color_Red      0x001F    //红色
#define Display_Color_Blue     0xF800    //蓝色
#define Display_Color_Green    0x07E0    //绿色
#define Display_Color_Yellow   0x07FF    //黄色
#define Display_Color_Magenta  0xF81F    //洋红
#define Display_Color_Cyan     0xFFE0    //青色
#define Display_Color_White    0xFFFF    //白色

#define DOT_RADIUS 3
#define BG_COLOR Display_Color_White
#define DOT_COLOR Display_Color_Red


static Adafruit_ST7735 tft =
  Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void lcd_init(void) {
  tft.initR(INITR_BLACKTAB);
  tft.setFont();
  tft.setTextSize(1);
  tft.fillScreen(LCD_BG);
  tft.setTextColor(LCD_TEXT, LCD_BG);
}

void lcd_enable(bool on) {
  tft.enableDisplay(on);
}

void lcd_clear(uint16_t color) {
  tft.fillScreen(color);
}

void lcd_set_cursor(int x, int y) {
  tft.setCursor(x, y);
}

void lcd_set_text_color(uint16_t fg, uint16_t bg) {
  tft.setTextColor(fg, bg);
}

void lcd_print(const char* text) {
  tft.print(text);
}
void lcd_draw_dot(int x, int y) {
    tft.fillCircle(x, y, DOT_RADIUS, DOT_COLOR);
}

void lcd_clear_dot(int x, int y) {
    tft.fillCircle(x, y, DOT_RADIUS, BG_COLOR);
}

