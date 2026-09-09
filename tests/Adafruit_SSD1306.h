#pragma once
#define SSD1306_WHITE 1
#define SSD1306_BLACK 0
class Adafruit_SSD1306 {
 public:
  void clearDisplay() {}
  void setTextSize(int) {}
  void setTextColor(int) {}
  void setCursor(int, int) {}
  template<class T> void print(T) {}
  void drawRect(int, int, int, int, int) {}
  void fillRect(int, int, int, int, int) {}
  void drawPixel(int, int, int) {}
  void drawLine(int, int, int, int, int) {}
  void display() {}
};
