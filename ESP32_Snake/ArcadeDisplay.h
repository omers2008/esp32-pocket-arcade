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
  void display() {
    if (!ready) return;
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
};
#endif
