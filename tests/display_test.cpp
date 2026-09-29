#define ARCADE_TEST_TFT
#include "../ESP32_Snake/ArcadeDisplay.h"
#include <cassert>
#include <iostream>
int main() {
  ArcadeDisplay d;
  d.display(); assert(tftTrace.sent.empty());
  assert(d.begin());
  assert(SPI.clock==18 && SPI.output==23 && SPI.input==-1 && SPI.chip==27);
  assert(tftTrace.cs==27 && tftTrace.dc==26 && tftTrace.rst==25);
  assert(tftTrace.initW==240 && tftTrace.initH==320 && tftTrace.rotation==1);
  assert(tftTrace.hz==20000000 && tftTrace.inverted);
  d.drawPixel(0,0,SSD1306_WHITE); d.drawPixel(127,63,SSD1306_WHITE);
  d.drawPixel(7,1,SSD1306_WHITE); d.drawPixel(8,1,SSD1306_INVERSE);
  d.drawPixel(8,1,SSD1306_INVERSE);
  d.display();
  assert(tftTrace.left==32 && tftTrace.top==56 && tftTrace.w==256 && tftTrace.h==128);
  assert(tftTrace.sent.size()==256*128 && !tftTrace.writing);
  for(int y=0;y<128;++y) for(int x=0;x<256;++x) {
    bool on=(x<2 && y<2) || (x>=254 && y>=126) || (x>=14 && x<16 && y>=2 && y<4);
    assert(tftTrace.sent[y*256+x]==(on?0xFFFF:0));
  }
  d.clearDisplay(); d.display(); for(auto p:tftTrace.sent) assert(p==0);
  const char *const names[]={"Snake","Pong","Tetris","Space Invaders","Skull Depths","Donkey Kong"};
  for(int selected=0;selected<6;++selected) {
    int before=tftTrace.transactions,clears=tftTrace.clears;
    assert(d.colorMenu(selected,6,names));
    assert(tftTrace.transactions==before+6 && tftTrace.top==200 && tftTrace.w==320 && tftTrace.h==40);
    assert(tftTrace.sent.size()==320*40 && tftTrace.clears==clears);
    d.display();assert(tftTrace.clears==clears+1);
    d.display();assert(tftTrace.clears==clears+1);
  }
  failCanvas=true;int tx=tftTrace.transactions;assert(!d.colorMenu(0,6,names));assert(tftTrace.transactions==tx);
  failCanvas=true; ArcadeDisplay failed; assert(!failed.begin());
  std::cout<<"PASS: TFT wiring, initialization, canvas scaling, bit boundaries, colors, clear and allocation failure.\n";
}
