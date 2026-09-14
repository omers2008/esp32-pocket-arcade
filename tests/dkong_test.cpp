#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/DonkeyKong.h"
#undef private
uint32_t testClock = 0;
using Game = DonkeyKong;
void tick(Game &g, int x=0, int y=0, bool jump=false) { testClock+=20; g.update(x,y,jump); }
void begin(Game &g,bool hard=false) { g.start(hard); tick(g,0,0,true); tick(g); g.spawnTimer=10000; }
void place(Game &g,int level,float x) { g.level=level; g.x=x; g.y=g.floorY(level,x); g.vy=0; g.jumping=false; g.climbTo=-1; }
int main() {
  Game g; Adafruit_SSD1306 d;
  g.start(false); g.draw(d); assert(!g.started && g.lives==3);
  tick(g,0,0,true); assert(g.started && !g.jumping);
  tick(g,0,0,true); assert(!g.jumping); tick(g);
  tick(g,0,0,true); assert(g.jumping);
  float peak=g.y;
  for(int i=0;i<35;++i) { tick(g,0,0,true); peak=min(peak,g.y); }
  assert(!g.jumping && peak < g.floorY(0,g.x)-5); // Holding doesn't auto-jump.
  begin(g); g.update(0,0,true); g.update(0,0,false); tick(g); assert(g.jumping);
  tick(g); tick(g,0,0,true); assert(g.vy > -1.5f); // No double jump.

  // Every ladder is usable in both directions; centered stick pauses climbing.
  for(int level=0;level<4;++level) for(int n=0;n<2;++n) {
    begin(g); place(g,level,float(g.ladderX(level,n)));
    tick(g,0,1000); assert(g.climbTo==level+1);
    float savedY=g.y; tick(g); assert(g.y==savedY);
    for(int i=0;i<35 && g.climbTo>=0;++i) tick(g,0,1000);
    assert(g.level==level+1 && g.climbTo<0);
    tick(g,0,-1000); assert(g.climbTo>=0);
    for(int i=0;i<35 && g.climbTo>=0;++i) tick(g,0,-1000);
    assert(g.level==level && g.climbTo<0);
  }
  begin(g); place(g,1,30); tick(g,0,1000); assert(g.climbTo<0);
  tick(g,1000); assert(abs(g.y-g.floorY(g.level,g.x))<0.001f);
  for(int i=0;i<300;++i) tick(g,-1000);
  assert(g.x>=15 && g.y>0 && g.y<64);

  // One barrel follows slopes, falls to the next girder, and eventually exits.
  begin(g); g.spawn(); auto &b=g.barrels[0]; assert(b.active && b.level==4);
  b.x=123; b.y=g.floorY(4,b.x); g.roll(b); assert(b.falling && b.level==3);
  for(int i=0;i<100 && b.falling;++i) g.roll(b);
  assert(!b.falling); float oldX=b.x; g.roll(b); assert(b.x<oldX);
  for(int i=0;i<3000 && b.active;++i) g.roll(b);
  assert(!b.active);
  // Grounded barrel contact loses a life; a clean jump earns one award.
  begin(g); g.immune=0; place(g,1,80);
  b={g.x,g.y,0,1,true,false,false}; tick(g); assert(g.lives==2 && g.level==0);
  begin(g); place(g,1,80); g.jumping=true; g.y-=6; g.vy=0; g.immune=0;
  b={g.x,g.floorY(1,g.x),0,1,true,false,false}; tick(g);
  assert(g.lives==3 && g.score==100 && b.jumped);
  tick(g); assert(g.score==100);

  // Hammers auto-pick up, smash barrels, expire, and prevent climbing/jumping.
  begin(g); place(g,1,91); tick(g); assert(g.hammer==250 && !g.hammers[0]);
  b={g.x+3,g.y,0,1,true,false,false}; tick(g,0,0,true);
  assert(!b.active && g.score==300 && !g.jumping);
  place(g,1,46); tick(g,0,1000); assert(g.climbTo<0);
  for(int i=0;i<250;++i) tick(g);
  assert(!g.hammer);
  // Oil fire contact and round timeout consume lives, including the last life.
  begin(g); g.immune=0; place(g,0,g.fireX); tick(g); assert(g.lives==2);
  g.lives=1; g.timeLeft=1; tick(g); assert(g.over && g.lives==0);
  // Rescue adds a time bonus once, pauses, then resets hazards for a harder round.
  begin(g); place(g,4,112); tick(g); assert(g.cleared==100 && g.score>=1000);
  uint32_t score=g.score; g.draw(d);
  for(int i=0;i<99;++i) tick(g,1000,0,true);
  assert(g.score==score && g.round==1);
  tick(g); assert(g.round==2 && g.level==0 && g.lives==3 && g.hammers[0]);
  g.draw(d);
  begin(g,true); assert(g.lives==2 && g.timeLeft<6000);
  // Follow a complete rescue route via actual movement and climbing input.
  begin(g); g.fireX=94; g.immune=10000;
  for(int level=0;level<4;++level) {
    int target=g.ladderX(level,0);
    for(int i=0;i<200 && abs(g.x-target)>1;++i) tick(g,g.x<target?1000:-1000);
    g.hammer=0; // Let a collected hammer expire before climbing.
    tick(g,0,1000);
    for(int i=0;i<35 && g.climbTo>=0;++i) tick(g,0,1000);
    assert(g.level==level+1);
  }
  for(int i=0;i<200 && !g.cleared;++i) tick(g,1000);
  assert(g.cleared);
  std::cout<<"PASS: Donkey Kong jump, ladders, slopes, barrels, hammers, fire, rescue, lives and difficulty.\n";
}
