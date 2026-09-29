#pragma once

// Existing host gameplay tests keep their lightweight drawing stub. The actual
// TFT adapter is exercised separately by display_test.cpp.
#if !defined(ARDUINO) && !defined(ARCADE_TEST_TFT)
#include <Adafruit_SSD1306.h>
using ArcadeDisplay = Adafruit_SSD1306;
#else
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "DisplayConfig.h"
#include "ColorMenu.h"

// Render full-resolution UI in small strips rather than a 150 KB framebuffer.
class MenuStripe : public Adafruit_GFX {
 public:
  MenuStripe(GFXcanvas16 &buffer,int top) : Adafruit_GFX(320,240),buffer(buffer),top(top) {}
  void drawPixel(int16_t x,int16_t y,uint16_t c) override { buffer.drawPixel(x,y-top,c); }
  void fillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t c) override {
    int a=max(int(y),top),b=min(int(y+h),top+40);
    if(b>a)buffer.fillRect(x,a-top,w,b-a,c);
  }
  void drawFastHLine(int16_t x,int16_t y,int16_t w,uint16_t c) override { fillRect(x,y,w,1,c); }
  void drawFastVLine(int16_t x,int16_t y,int16_t h,uint16_t c) override { fillRect(x,y,1,h,c); }
 private:
  GFXcanvas16 &buffer;
  int top;
};

// Legacy game color values are bits in the logical canvas, not RGB565 values.
constexpr uint16_t SSD1306_BLACK = 0;
constexpr uint16_t SSD1306_WHITE = 1;
constexpr uint16_t SSD1306_INVERSE = 2;

class ArcadeDisplay : public GFXcanvas1 {
 public:
  ArcadeDisplay() : GFXcanvas1(ArcadeScreen::WIDTH, ArcadeScreen::HEIGHT),
    panel(&SPI, ArcadeScreen::CS, ArcadeScreen::DC, ArcadeScreen::RESET) {}

  bool begin() {
    if (!getBuffer()) return false;
    SPI.begin(ArcadeScreen::SCLK, -1, ArcadeScreen::MOSI, ArcadeScreen::CS);
    panel.init(240, 320);
    panel.setRotation(ArcadeScreen::ROTATION);
    panel.setSPISpeed(ArcadeScreen::SPI_HZ);
    panel.invertDisplay(ArcadeScreen::INVERT);
    panel.fillScreen(ArcadeScreen::BACKGROUND);
    clearDisplay();
    ready = true;
    return true; // Write-only SPI cannot confirm that a screen is connected.
  }

  void clearDisplay() { fillScreen(SSD1306_BLACK); }
  bool colorMenu(int selected,int count,const char *const *names) {
    if(!ready)return false;
    GFXcanvas16 strip(320,40);
    if(!strip.getBuffer())return false;
    for(int top=0;top<240;top+=40) {
      MenuStripe viewport(strip,top);
      ColorMenu::draw(viewport,selected,count,names);
      panel.startWrite();panel.setAddrWindow(0,top,320,40);
      panel.writePixels(strip.getBuffer(),320*40,true);panel.endWrite();
    }
    nativeMenu = true;
    return true;
  }
  void display() {
    if (!ready) return;
    if(nativeMenu) { panel.fillScreen(ArcadeScreen::BACKGROUND);nativeMenu=false; }
    constexpr int scaledW = ArcadeScreen::WIDTH * ArcadeScreen::SCALE;
    constexpr int scaledH = ArcadeScreen::HEIGHT * ArcadeScreen::SCALE;
    const int left = (panel.width() - scaledW) / 2;
    const int top = (panel.height() - scaledH) / 2;
    panel.startWrite();
    panel.setAddrWindow(left, top, scaledW, scaledH);
    for (int y = 0; y < ArcadeScreen::HEIGHT; ++y) {
      // One small scanline, no full RGB framebuffer. Blocking writes let us
      // reuse it safely, keeping RAM available to all 23 games.
      for (int x = 0; x < ArcadeScreen::WIDTH; ++x) {
        uint16_t color = getPixel(x, y) ? ArcadeScreen::FOREGROUND : ArcadeScreen::BACKGROUND;
        for (int dx = 0; dx < ArcadeScreen::SCALE; ++dx) row[x * ArcadeScreen::SCALE + dx] = color;
      }
      for (int dy = 0; dy < ArcadeScreen::SCALE; ++dy) panel.writePixels(row, scaledW, true);
    }
    panel.endWrite();
  }

 private:
  Adafruit_ST7789 panel;
  uint16_t row[ArcadeScreen::WIDTH * ArcadeScreen::SCALE] = {};
  bool ready = false;
  bool nativeMenu = false;
};
#endif
