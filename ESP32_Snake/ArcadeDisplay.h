#pragma once
#include "GameColors.h"

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
  void setGame(int game,const char *name=nullptr,bool hard=false) {
    gameId=game;gameName=name;hardMode=hard;
    if(game<0)chromeGame=-1;
  }
  void drawPixel(int16_t x,int16_t y,uint16_t color) override {
    if(x<0 || x>=128 || y<0 || y>=64)return;
    if(color==SSD1306_INVERSE) pixels[y*128+x]^=0xFFFF;
    else pixels[y*128+x]=color==SSD1306_WHITE?Ink::White:color;
    GFXcanvas1::drawPixel(x,y,color>2?SSD1306_WHITE:color);
  }
  void fillScreen(uint16_t color) {
    GFXcanvas1::fillScreen(color>2?SSD1306_WHITE:color);
    uint16_t rgb=color==SSD1306_WHITE?Ink::White:color;
    for(auto &pixel:pixels)pixel=rgb;
  }
  void fillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t color) override {
    int right=min(128,int(x)+w),bottom=min(64,int(y)+h);
    for(int yy=max(0,int(y));yy<bottom;++yy)
      for(int xx=max(0,int(x));xx<right;++xx)drawPixel(xx,yy,color);
  }
  void drawFastHLine(int16_t x,int16_t y,int16_t w,uint16_t c) override {fillRect(x,y,w,1,c);}
  void drawFastVLine(int16_t x,int16_t y,int16_t h,uint16_t c) override {fillRect(x,y,1,h,c);}
  bool colorMenu(int selected,int count,const char *const *names) {
    return colorFrame([&](Adafruit_GFX &viewport) { ColorMenu::draw(viewport,selected,count,names); });
  }
  template<class Painter> bool colorFrame(Painter paint) {
    if(!ready)return false;
    GFXcanvas16 strip(320,40);
    if(!strip.getBuffer())return false;
    for(int top=0;top<240;top+=40) {
      MenuStripe viewport(strip,top);
      paint(viewport);
      panel.startWrite();panel.setAddrWindow(0,top,320,40);
      panel.writePixels(strip.getBuffer(),320*40,true);panel.endWrite();
    }
    nativeMenu = true;
    return true;
  }
  void display() {
    if (!ready) return;
    if(gameId>=0) {
      bool repaint=nativeMenu || chromeGame!=gameId || chromeHard!=hardMode;
      if(repaint) {
        if(!colorFrame([&](Adafruit_GFX &d) {
          d.fillScreen(ColorMenu::BG);d.setTextWrap(false);
          ColorMenu::label(d,10,10,gameName,ColorMenu::accent(gameId),2);
          ColorMenu::label(d,10,30,hardMode?"HARD":"EASY",ColorMenu::MUTED);
          d.drawFastHLine(0,42,320,ColorMenu::accent(gameId));
          d.drawFastHLine(0,205,320,ColorMenu::accent(gameId));
          ColorMenu::label(d,10,215,controls(gameId),ColorMenu::WHITE);
          ColorMenu::label(d,10,229,"12  GAME MENU",ColorMenu::MUTED);
        }))return;
        chromeGame=gameId;chromeHard=hardMode;nativeMenu=false;
      }
      // Uniform 2.5x scaling preserves geometry. Only changed row spans are
      // transferred, keeping moving sprites responsive at the existing SPI rate.
      panel.startWrite();
      for(int y=0;y<64;++y) {
        int first=0,last=127;
        if(!repaint) {
          while(first<128 && pixels[y*128+first]==previous[y*128+first])++first;
          while(last>=first && pixels[y*128+last]==previous[y*128+last])--last;
        }
        if(last<first)continue;
        int left=(first*5+1)/2,right=((last+1)*5+1)/2;
        int top=(y*5+1)/2,bottom=((y+1)*5+1)/2;
        for(int x=left;x<right;++x)row[x-left]=pixels[y*128+x*2/5];
        panel.setAddrWindow(left,44+top,right-left,bottom-top);
        for(int repeat=top;repeat<bottom;++repeat)panel.writePixels(row,right-left,true);
        for(int x=first;x<=last;++x)previous[y*128+x]=pixels[y*128+x];
      }
      panel.endWrite();return;
    }
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
  static const char *controls(int game) {
    static const char *const hints[]={
      "STICK  MOVE", "STICK  MOVE   13  FIRE", "STICK  PADDLE   13  SERVE",
      "13  ROTATE   14  HOLD", "STICK  MOVE   13  ATTACK   14  JUMP",
      "STICK  AIM   13  SHOOT", "STICK  MOVE", "13  HIT   14  STAND",
      "STICK  MOVE/JUMP   13  PUNCH   14  KICK", "13  PLAY   14  END TURN",
      "STICK  MOVE/CLIMB   13  THROW   14  JUMP", "13  BET+   14  BET-   DOWN  SPIN",
      "STICK  COLUMN   13  DROP", "STICK  SQUARE   13  PLACE",
      "STICK  MOVE   13  DIG   14  FLAG", "13  LEFT FLIPPER   14  RIGHT FLIPPER",
      "STICK  MOVE   13  ATTACK   14  ITEM", "STICK  DRIVE   13  FIRE   14  RICOCHET",
      "13/UP  JUMP   14/DOWN  DUCK", "STICK  STEER/THRUST   13  FIRE   14  WARP",
      "STICK  FLY   13  BOOST   14  FIRE", "STICK  MOVE   13  SWORD   14  DASH",
      "STICK  MOVE/CLIMB   13  JUMP"};
    return hints[game%23];
  }
  Adafruit_ST7789 panel;
  uint16_t row[320] = {};
  uint16_t pixels[128*64] = {};
  uint16_t previous[128*64] = {};
  int gameId=-1,chromeGame=-1;
  const char *gameName=nullptr;
  bool hardMode=false,chromeHard=false;
  bool ready = false;
  bool nativeMenu = false;
};
#endif
