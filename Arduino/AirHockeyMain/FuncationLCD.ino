#include "Global.h"

void lcd_init() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  Serial.println("LCD Ready!");
}

void lcd_print_text(int x, int y, const String &text) {
  tft.setCursor(x, y);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(2);
  tft.println(text);
}

void lcd_clear() {
  tft.fillScreen(ILI9341_BLACK);
}


// Touch screen functions

void touch_init() {
  ts.begin();

  ts.setRotation(3);

  Serial.println("Touch Ready!");
}

TouchPoint getTouchPoint() {
  TouchPoint result;
  result.pressed = false;
  result.x = 0;
  result.y = 0;

  if (!ts.touched()) {
    return result;
  }

  TS_Point p = ts.getPoint();

  int rawX = p.x;
  int rawY = p.y;

  // Invalid values - not counted as a press
  if ((rawX >= 4090 && rawY >= 4090) ||
      (rawX <= 5 && rawY <= 5)) {
    return result;
  }

  int x = round(435.86 - 0.1081 * rawX - 0.02734 * rawY);
  int y = round(345.85 - 0.01815 * rawX - 0.09268 * rawY);

  x = constrain(x, 0, 319);
  y = constrain(y, 0, 239);

  result.pressed = true;
  result.x = x;
  result.y = y;

  delay(180);
  return result;
}

bool pointInRect(TouchPoint p, int x, int y, int w, int h) {
  if (!p.pressed) {
    return false;
  }

  return p.x >= x &&
         p.x <= x + w &&
         p.y >= y &&
         p.y <= y + h;
}

void debugTouchPoint() {
  TouchPoint p = getTouchPoint();

  if (p.pressed) {
    Serial.print("Pressed calibrated x=");
    Serial.print(p.x);
    Serial.print(" y=");
    Serial.println(p.y);
    delay(250);
  }
}