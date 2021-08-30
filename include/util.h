#pragma once

#include <M5Core2.h>

void draw_center_center_string(char *str) {
  int sw = m5.lcd.textWidth(str);
  int sh = m5.lcd.fontHeight();

  int lcd_center_x = m5.lcd.width() / 2;
  int lcd_center_y = m5.lcd.height() / 2;

  m5.lcd.drawString(str, lcd_center_x - (sw / 2), lcd_center_y - (sh / 2));
}
