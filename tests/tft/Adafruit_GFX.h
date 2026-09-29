#pragma once
#include <cstdint>
#include <cstring>
inline bool failCanvas = false;
class GFXcanvas1 {
 public:
  GFXcanvas1(int,int) {}
  uint8_t *getBuffer() { return failCanvas ? nullptr : bits; }
  void fillScreen(uint16_t color) { std::memset(bits, color ? 255 : 0, sizeof(bits)); }
  bool getPixel(int x,int y) const { return bits[y*16+x/8] & (0x80>>(x%8)); }
  void drawPixel(int x,int y,uint16_t color) {
    uint8_t &b=bits[y*16+x/8], mask=0x80>>(x%8);
    if(color==2) b^=mask; else if(color) b|=mask; else b&=~mask;
  }
 private:
  uint8_t bits[1024] = {};
};
