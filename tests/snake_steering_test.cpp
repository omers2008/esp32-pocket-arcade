#include "../ESP32_Snake/SnakeSteering.h"
#include <cassert>
#include <iostream>

int main() {
  SnakeSteering input;
  Direction wanted = RIGHT;
  assert(!input.read(900, -1100, 0, wanted));
  assert(!input.read(1150, -1050, 15, wanted));
  assert(input.read(1150, -1050, 25, wanted) && wanted == DOWN);
  for (uint32_t t=30; t<200; t+=10) {
    assert(input.read(t%20 ? 1150 : 1000, -1100, t, wanted));
    assert(wanted == DOWN); // Near-diagonal noise must not create stairs.
  }
  assert(!input.read(1800, -700, 200, wanted));
  assert(input.read(1800, -700, 225, wanted) && wanted == RIGHT);
  assert(input.read(1050, -1150, 250, wanted) && wanted == RIGHT);
  assert(!input.read(0, 0, 275, wanted));
  assert(!input.read(-1500, 0, 300, wanted));
  assert(input.read(-1500, 0, 325, wanted) && wanted == LEFT);
  assert(!input.read(0, 1500, 350, wanted));
  assert(input.read(0, 1500, 375, wanted) && wanted == UP);
  input.reset();
  assert(!input.read(1500, -700, 400, wanted));
  assert(input.read(1500, -700, 425, wanted) && wanted == RIGHT);
  assert(input.read(1500, 700, 430, wanted) && wanted == RIGHT); // Y sign noise stays horizontal.
  std::cout << "PASS: down/right stability, deliberate axis changes, all directions and settling.\n";
}
