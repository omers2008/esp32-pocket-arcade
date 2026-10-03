#include "../ESP32_Snake/ServoInput.h"
#include <cassert>
#include <iostream>
int main(){
  ServoInput input;
  assert(input.update(1200,false)=='S' && !input.armed);
  assert(input.update(-1200,false)=='S' && !input.armed);
  assert(input.update(0,false)=='S' && input.armed);
  assert(input.update(650,false)=='S');
  assert(input.update(-650,false)=='S');
  assert(input.update(1000,false)=='S');
  assert(input.update(-1000,false)=='S');
  assert(input.update(1001,false)=='R');
  assert(input.update(800,false)=='R');
  assert(input.update(750,false)=='S');
  assert(input.update(-1001,false)=='L');
  assert(input.update(-800,false)=='L');
  assert(input.update(-750,false)=='S');
  assert(input.update(1200,false)=='R');
  assert(input.update(-1200,false)=='L');
  assert(input.update(-1200,true)=='S' && !input.armed);
  assert(input.update(-1200,false)=='S');
  assert(input.update(0,false)=='S' && input.armed);
  assert(input.update(1200,false)=='R');
  input.reset();assert(input.command=='S' && !input.armed);
  std::cout<<"PASS: servo center-to-arm, direction mapping, hysteresis, stop latch and reset.\n";
}
