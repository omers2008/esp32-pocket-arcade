#include "../ESP32_Snake/FloorInput.h"
#include <cassert>
#include <iostream>
int main(){
  FloorInput f;
  auto move=[&](int x,int y){f.update(0,0);f.update(x,y);};
  f.update(1500,0);assert(f.floor==1);
  f.update(0,0);f.update(1000,-1000);assert(f.floor==1);
  f.update(1001,0);assert(f.floor==2);
  for(int i=0;i<20;++i)f.update(1500,0);assert(f.floor==2);
  move(0,-1500);assert(f.floor==7);
  move(0,1500);assert(f.floor==2);
  move(-1500,0);assert(f.floor==1);
  move(-1500,0);assert(f.floor==1);
  move(0,1500);assert(f.floor==1);
  for(int target=2;target<=5;++target){move(1500,0);assert(f.floor==target);}
  move(1500,0);assert(f.floor==5);
  move(0,-1500);assert(f.floor==10);
  move(0,-1500);assert(f.floor==10);
  for(int target=9;target>=6;--target){move(-1500,0);assert(f.floor==target);}
  move(-1500,0);assert(f.floor==6);
  move(1200,1600);assert(f.floor==1); // Dominant vertical axis.
  move(1600,-1200);assert(f.floor==2); // Dominant horizontal axis.
  f.update(0,751);assert(!f.centered);
  f.update(-1500,0);assert(f.floor==2);
  f.update(750,-750);assert(f.centered);
  f.update(900,0);assert(f.centered && f.floor==2);
  f.update(-1001,0);assert(f.floor==1);
  f.reset();assert(f.floor==1 && !f.centered);
  std::cout<<"PASS: all ten grid cells, clamped edges, four directions, dominant axis, center gating and widened deadzone.\n";
}
