#pragma once
#include <Adafruit_GFX.h>

// Native 320x240 menu. Game drawing continues to use the 128x64 canvas.
namespace ColorMenu {
constexpr uint16_t BG=0x0863, CARD=0x10C6, ACTIVE=0x1970;
constexpr uint16_t WHITE=0xF7BE, MUTED=0x8CB3, CYAN=0x2E9F, GOLD=0xFE89;
inline uint16_t accent(int game) {
  static const uint16_t palette[]={0x5F54,0x2E9F,0xFD6B,0xB47F,0xFAAE,0xFE89};
  return palette[game%6];
}
inline void label(Adafruit_GFX &d,int x,int y,const char *s,uint16_t color,int size=1) {
  d.setTextSize(size); d.setTextColor(color); d.setCursor(x,y); d.print(s);
}
inline void icon(Adafruit_GFX &d,int game,int x,int y,uint16_t color) {
  // Hand-drawn pixel badges, grouped by the game's recognizable mechanics.
  if(game==23) { // Block Breaker.
    for(int row=0;row<2;++row)for(int col=0;col<3;++col)d.fillRect(x+col*7,y+row*5,6,4,color);
    d.fillCircle(x+11,y+13,2,GOLD);d.fillRect(x+4,y+18,13,2,CYAN);
  } else if(game==0) { // Snake.
    d.fillRect(x,y+3,15,4,color);d.fillRect(x,y+3,4,14,color);
    d.fillRect(x,y+13,12,4,color);d.fillRect(x+12,y+1,7,7,color);
    d.drawPixel(x+17,y+2,BG);d.fillRect(x+16,y+15,3,3,GOLD);
  } else if(game==3) { // Tetromino.
    for(int i=0;i<3;++i)d.fillRect(x+i*6,y+9,5,5,color);
    d.fillRect(x+6,y+3,5,5,color);
  } else if(game==2 || game==15) {
    d.fillRect(x,y+2,3,16,color);d.fillRect(x+17,y+2,3,16,color);
    d.fillRect(x+9,y+8,4,4,WHITE);
  } else if(game==6) {
    d.fillCircle(x+8,y+10,8,color);
    d.fillTriangle(x+8,y+10,x+17,y+4,x+17,y+16,CARD);
    d.fillRect(x+18,y+9,2,2,GOLD);
  } else if(game==7 || game==9 || game==11) {
    d.drawRoundRect(x+1,y+1,14,18,2,color);
    d.fillTriangle(x+8,y+5,x+4,y+10,x+12,y+10,color);
    d.fillTriangle(x+8,y+15,x+4,y+10,x+12,y+10,color);
  } else if(game==12 || game==13 || game==14) {
    d.drawRect(x,y,20,20,color);
    d.drawFastVLine(x+7,y,20,color);d.drawFastVLine(x+13,y,20,color);
    d.drawFastHLine(x,y+7,20,color);d.drawFastHLine(x,y+13,20,color);
    d.fillCircle(x+10,y+10,2,GOLD);
  } else if(game==4 || game==8 || game==10 || game==16 || game==21) {
    d.drawLine(x+3,y+17,x+16,y+3,color);d.drawLine(x+4,y+17,x+17,y+3,color);
    d.drawLine(x+2,y+10,x+10,y+18,GOLD);d.fillRect(x+15,y+1,4,4,color);
  } else if(game==18 || game==22) {
    d.fillRect(x+7,y+2,10,7,color);d.fillRect(x+3,y+8,10,6,color);
    d.fillRect(x+3,y+14,3,5,color);d.fillRect(x+10,y+14,3,5,color);d.drawPixel(x+14,y+4,BG);
  } else { // Ships / aiming games.
    d.fillTriangle(x+10,y+1,x+1,y+17,x+19,y+17,color);
    d.fillRect(x+8,y+8,4,6,BG);d.fillRect(x+8,y+18,4,2,GOLD);
  }
}
inline void draw(Adafruit_GFX &d,int selected,int count,const char *const *names) {
  d.fillScreen(BG); d.setTextWrap(false);
  label(d,12,10,"POCKET",WHITE,2);label(d,94,10,"ARCADE",CYAN,2);
  d.fillRoundRect(257,8,51,22,5,CARD);
  d.setTextSize(1);d.setTextColor(WHITE);d.setCursor(268,15);
  d.print(selected+1);d.print('/');d.print(count);
  label(d,13,33,"CHOOSE YOUR NEXT ADVENTURE",MUTED);
  int first=max(0,min(selected-2,count-5));
  for(int slot=0;slot<5 && first+slot<count;++slot) {
    int game=first+slot,y=48+slot*33;bool active=game==selected;
    uint16_t tint=accent(game);
    d.fillRoundRect(10,y,294,30,5,active?ACTIVE:CARD);
    if(active){d.drawRoundRect(10,y,294,30,5,tint);d.fillRoundRect(10,y+5,3,20,1,tint);}
    icon(d,game,21,y+5,tint);
    label(d,52,y+7,names[game],active?WHITE:MUTED,2);
    if(active)d.fillTriangle(292,y+10,297,y+15,292,y+20,tint);
  }
  d.fillRoundRect(310,48,3,162,1,CARD);
  d.fillRoundRect(310,48+(selected*142/max(1,count-1)),3,20,1,CYAN);
  d.drawFastHLine(12,217,296,CARD);
  label(d,13,226,"STICK  SCROLL",MUTED);
  d.fillRoundRect(185,221,123,17,4,CYAN);
  label(d,200,226,"13  SELECT GAME",BG);
}
}
