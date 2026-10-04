#include "../ESP32_Snake/FloorInput.h"
#include <cassert>
#include <iostream>
int main(){
  FloorInput f;
  f.update(1500);assert(f.floor==1);
  f.update(0);f.update(1000);assert(f.floor==1);
  f.update(1001);assert(f.floor==2);
  for(int i=0;i<20;++i)f.update(1500);assert(f.floor==2);
  for(int i=0;i<20;++i){f.update(0);f.update(1500);}assert(f.floor==10);
  for(int i=0;i<20;++i){f.update(0);f.update(-1500);}assert(f.floor==1);
  f.update(750);assert(f.centered);f.update(900);assert(f.centered && f.floor==1);
  f.update(1001);assert(f.floor==2);f.reset();assert(f.floor==1 && !f.centered);
  std::cout<<"PASS: floors clamp to 1-10, center gating, widened deadzone and one step per tilt.\n";
}
