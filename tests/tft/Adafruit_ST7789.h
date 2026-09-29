#pragma once
#include "SPI.h"
#include <vector>
#include <cassert>
struct TFTTrace {
  int cs=-1,dc=-1,rst=-1,initW=0,initH=0,rotation=0,left=0,top=0,w=0,h=0;
  uint32_t hz=0;
  bool inverted=false,writing=false;
  std::vector<uint16_t> sent;
};
inline TFTTrace tftTrace;
class Adafruit_ST7789 {
 public:
  Adafruit_ST7789(SPIClass*,int cs,int dc,int rst) { tftTrace.cs=cs;tftTrace.dc=dc;tftTrace.rst=rst; }
  void init(int w,int h) { tftTrace.initW=w;tftTrace.initH=h; }
  void setRotation(int r) { tftTrace.rotation=r; }
  void setSPISpeed(uint32_t hz) { tftTrace.hz=hz; }
  void invertDisplay(bool b) { tftTrace.inverted=b; }
  void fillScreen(uint16_t) {}
  int width() const { return 320; }
  int height() const { return 240; }
  void startWrite() { assert(!tftTrace.writing);tftTrace.writing=true;tftTrace.sent.clear(); }
  void setAddrWindow(int x,int y,int w,int h) { tftTrace.left=x;tftTrace.top=y;tftTrace.w=w;tftTrace.h=h; }
  void writePixels(uint16_t *pixels,int count,bool block) { assert(block && tftTrace.writing);tftTrace.sent.insert(tftTrace.sent.end(),pixels,pixels+count); }
  void endWrite() { assert(tftTrace.writing);tftTrace.writing=false; }
};
