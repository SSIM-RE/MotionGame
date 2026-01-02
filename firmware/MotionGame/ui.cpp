#include "ui.h"
#include "lcd.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

static char oldTime[16] = {0};

void ui_init(void) {
  lcd_clear(0x07FF);
}

void ui_draw_uptime(void) {
  unsigned long sec = millis() / 1000;

  unsigned long d = sec / 86400;
  sec %= 86400;
  unsigned long h = sec / 3600;
  sec %= 3600;
  unsigned long m = sec / 60;
  sec %= 60;

  char buf[16];
  sprintf(buf, "%lu %02lu:%02lu:%02lu", d, h, m, sec);

  if (strcmp(buf, oldTime) != 0) {
    lcd_set_cursor(10, 80);
    lcd_print(oldTime);   // 擦除
    lcd_set_cursor(10, 80);
    lcd_print(buf);
    strcpy(oldTime, buf);
  }
}
