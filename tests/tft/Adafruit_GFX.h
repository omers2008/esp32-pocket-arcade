#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>
#include <sstream>
#include <cmath>
#ifdef ARCADE_RENDER_PREVIEW
#include <glcdfont.c>
#endif
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
  void setTextSize(int s) {size=s;} void setTextColor(uint16_t c) {ink=c;}
  void setTextWrap(bool v) {wrap=v;} void setCursor(int x,int y) {cx=x;cy=y;}
  template<class T>void print(T value) {
#ifdef ARCADE_RENDER_PREVIEW
    std::ostringstream out;
    if constexpr(std::is_same_v<T,uint8_t> || std::is_same_v<T,int8_t>)out<<int(value);
    else out<<value;
    for(unsigned char c:out.str()) {
      if(c=='\n'){cx=0;cy+=8*size;continue;}
      if(wrap && cx+6*size>w){cx=0;cy+=8*size;}
      for(int x=0;x<5;++x)for(int y=0;y<8;++y)
        if(font[int(c)*5+x]&(1<<y))fillRect(cx+x*size,cy+y*size,size,size,ink);
      cx+=6*size;
    }
#endif
  }
  void fillRoundRect(int x,int y,int a,int b,int r,uint16_t c) {
    fillRect(x+r,y,a-2*r,b,c);fillRect(x,y+r,a,b-2*r,c);
    fillCircle(x+r,y+r,r,c);fillCircle(x+a-r-1,y+r,r,c);
    fillCircle(x+r,y+b-r-1,r,c);fillCircle(x+a-r-1,y+b-r-1,r,c);
  }
  void drawRoundRect(int x,int y,int a,int b,int r,uint16_t c) {
    drawFastHLine(x+r,y,a-2*r,c);drawFastHLine(x+r,y+b-1,a-2*r,c);
    drawFastVLine(x,y+r,b-2*r,c);drawFastVLine(x+a-1,y+r,b-2*r,c);
    for(int dy=0;dy<=r;++dy){int dx=int(std::round(std::sqrt(double(r*r-dy*dy))));
      drawPixel(x+r-dx,y+r-dy,c);drawPixel(x+a-r-1+dx,y+r-dy,c);
      drawPixel(x+r-dx,y+b-r-1+dy,c);drawPixel(x+a-r-1+dx,y+b-r-1+dy,c);}
  }
  void drawRect(int x,int y,int a,int b,uint16_t c) {
    drawFastHLine(x,y,a,c);drawFastHLine(x,y+b-1,a,c);
    drawFastVLine(x,y,b,c);drawFastVLine(x+a-1,y,b,c);
  }
  void drawLine(int x,int y,int xx,int yy,uint16_t c) {
    int dx=std::abs(xx-x),sx=x<xx?1:-1,dy=-std::abs(yy-y),sy=y<yy?1:-1,err=dx+dy;
    for(;;){drawPixel(x,y,c);if(x==xx && y==yy)break;int e=2*err;
      if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}
  }
  void fillTriangle(int x,int y,int a,int b,int u,int v,uint16_t c) {
    auto edge=[](int x,int y,int a,int b,int u,int v){return (u-x)*(b-y)-(v-y)*(a-x);};
    for(int py=min(y,min(b,v));py<=max(y,max(b,v));++py)
      for(int px=min(x,min(a,u));px<=max(x,max(a,u));++px){
        int p=edge(x,y,a,b,px,py),q=edge(a,b,u,v,px,py),r=edge(u,v,x,y,px,py);
        if((p>=0 && q>=0 && r>=0)||(p<=0 && q<=0 && r<=0))drawPixel(px,py,c);
      }
  }
  void fillCircle(int x,int y,int r,uint16_t c) {
    for(int dy=-r;dy<=r;++dy){int dx=int(std::sqrt(double(r*r-dy*dy)));drawFastHLine(x-dx,y+dy,dx*2+1,c);}
  }
  void drawCircle(int x,int y,int r,uint16_t c) {
    int a=r,b=0,err=1-r;
    while(a>=b){drawPixel(x+a,y+b,c);drawPixel(x+b,y+a,c);drawPixel(x-b,y+a,c);drawPixel(x-a,y+b,c);
      drawPixel(x-a,y-b,c);drawPixel(x-b,y-a,c);drawPixel(x+b,y-a,c);drawPixel(x+a,y-b,c);
      ++b;if(err<0)err+=2*b+1;else{--a;err+=2*(b-a)+1;}}
  }
 private:
  int w,h;
  int cx=0,cy=0,size=1;uint16_t ink=0xFFFF;bool wrap=true;
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
  void drawPixel(int16_t x,int16_t y,uint16_t color) override {
    if(x<0 || x>=128 || y<0 || y>=64)return;
    uint8_t &b=bits[y*16+x/8], mask=0x80>>(x%8);
    if(color==2) b^=mask; else if(color) b|=mask; else b&=~mask;
  }
 private:
  uint8_t bits[1024] = {};
};
