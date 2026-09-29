#define ARCADE_TEST_TFT
#include <Arduino.h>
#include "../ESP32_Snake/ArcadeDisplay.h"
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/SkullDepths.h"
#undef private
uint32_t testClock=0;
int main(){
  ArcadeDisplay d;assert(d.begin());SkullDepths g;g.start(false);
  for(int state=0;state<4;++state){
    g.phase=static_cast<SkullDepths::Phase>(state);g.choices[0]=0;g.choices[1]=1;g.choices[2]=2;
    g.gold=100;g.selection=2;g.enemies[0].windup=12;g.enemies[1].archer=true;g.enemies[1].windup=12;
    auto frame=g.frames;float x=g.x;int hp=g.hp,gold=g.gold;int tx=tftTrace.transactions;
    g.draw(d);assert(tftTrace.transactions==tx+6);
    assert(g.frames==frame && g.x==x && g.hp==hp && g.gold==gold);
    assert(tftTrace.screen.size()==320*240 && !tftTrace.writing);
  }
  std::cout<<"PASS: Skull Depths native combat/intro/reward/shop renders preserve simulation state.\n";
}
