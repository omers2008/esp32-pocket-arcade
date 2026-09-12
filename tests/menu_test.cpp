#include <Arduino.h>
#include <cassert>
#include <iostream>
#include "../ESP32_Snake/MenuScroll.h"
uint32_t testClock = 0;
int main() {
  MenuScroll m;
  assert(m.update(0, -1000) == 1);
  testClock = 399; assert(m.update(0, -1000) == 0);
  testClock = 400; assert(m.update(0, -1000) == 1);
  testClock = 529; assert(m.update(0, -1000) == 0);
  testClock = 530; assert(m.update(0, -1000) == 1);
  assert(m.update(0, 0) == 0);
  testClock += 1000; assert(m.update(0, 0) == 0);
  assert(m.update(0, 1000) == -1);
  testClock += 400; assert(m.update(0, 1000) == -1);
  assert(m.update(0, -1000) == 1); // Immediate reversal.
  assert(m.update(1000, 0) == 1);
  testClock += 1000; assert(m.update(1000, 0) == 0); // Horizontal taps remain single-step.
  m.reset(); assert(m.update(0, -1000) == 0);
  testClock += 1000; assert(m.update(0, -1000) == 0);
  assert(m.update(0, 0) == 0); assert(m.update(0, -1000) == 1);
  m.update(0, 0); testClock = UINT32_MAX - 100;
  assert(m.update(0, 1000) == -1);
  testClock += 400; assert(m.update(0, 1000) == -1);
  assert(m.update(0, 500) == 0); // Stop as soon as the stick leaves the active zone.
  std::cout << "PASS: menu tap, hold delay/repeat, release, reversal, reset/center gating, horizontal taps and timer rollover.\n";
}
