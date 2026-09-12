#pragma once
#include <Arduino.h>

// Game-list navigation: tap once, then repeat vertical holds after 400 ms.
class MenuScroll {
 public:
  void reset() { armed = false; direction = 0; repeating = false; }
  int update(int x, int y) {
    if (abs(x) < 350 && abs(y) < 350) armed = true;
    bool vertical = abs(y) >= abs(x);
    int wanted = max(abs(x), abs(y)) <= 650 ? 0 :
      vertical ? (y > 0 ? -1 : 1) : (x > 0 ? 1 : -1);
    if (!wanted || !armed) { direction = 0; repeating = false; return 0; }
    uint32_t now = millis();
    if (wanted != direction || vertical != wasVertical) {
      direction = wanted; wasVertical = vertical; lastStep = now; repeating = false;
      return wanted;
    }
    if (vertical && now - lastStep >= uint32_t(repeating ? 130 : 400)) {
      lastStep = now; repeating = true; return wanted;
    }
    return 0;
  }
 private:
  bool armed = true, repeating = false, wasVertical = false;
  int direction = 0;
  uint32_t lastStep = 0;
};
