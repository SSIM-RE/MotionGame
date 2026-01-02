#pragma once
#include <stdint.h>
#include <stdbool.h>

void lcd_init(void);
void lcd_enable(bool on);

void lcd_clear(uint16_t color);
void lcd_set_cursor(int x, int y);
void lcd_set_text_color(uint16_t fg, uint16_t bg);
void lcd_print(const char* text);
void lcd_draw_dot(int x, int y);
void lcd_clear_dot(int x, int y);