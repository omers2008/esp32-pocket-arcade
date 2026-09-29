#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>
using std::min;
using std::max;
inline bool failCanvas = false;
class Adafruit_GFX {
 public:
  Adafruit_GFX(int w,int h):w(w),h(h) {}
  virtual void drawPixel(int16_t,int16_t,uint16_t) {}
  virtual void fillRect(int16_t x,int16_t y,int16_t width,int16_t height,uint16_t c) {
    for(int j=y;j<y+height;++j)for(int i=x;i<x+width;++i)drawPixel(i,j,c);
  }
  virtual void drawFastHLine(int16_t x,int16_t y,int16_t n,uint16_t c) {fillRect(x,y,n,1,c);}
  virtual void drawFastVLine(int16_t x,int16_t y,int16_t n,uint16_t c) {fillRect(x,y,1,n,c);}
  void fillScreen(uint16_t c) {fillRect(0,0,w,h,c);}
  void setTextSize(int) {} void setTextColor(uint16_t) {} void setTextWrap(bool) {} void setCursor(int,int) {}
  template<class T>void print(T) {}
  void fillRoundRect(int x,int y,int a,int b,int,uint16_t c) {fillRect(x,y,a,b,c);}
  void drawRoundRect(int,int,int,int,int,uint16_t) {}
  void drawRect(int,int,int,int,uint16_t) {}
  void drawLine(int,int,int,int,uint16_t) {}
  void fillTriangle(int,int,int,int,int,int,uint16_t) {}
  void fillCircle(int,int,int,uint16_t) {}
 private:
  int w,h;
};
class GFXcanvas16 : public Adafruit_GFX {
 public:
  GFXcanvas16(int w,int h):Adafruit_GFX(w,h),w(w),h(h),pixels(w*h) {}
  uint16_t *getBuffer() {return failCanvas?nullptr:pixels.data();}
  void drawPixel(int16_t x,int16_t y,uint16_t c) override {if(x>=0 && x<w && y>=0 && y<h)pixels[y*w+x]=c;}
 private:
  int w,h;std::vector<uint16_t> pixels;
};
class GFXcanvas1 : public Adafruit_GFX {
 public:
  GFXcanvas1(int w,int h) : Adafruit_GFX(w,h) {}
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
