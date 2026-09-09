#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/SlotMachine.h"
#undef private

uint32_t testClock = 0;

void advance(SlotMachine &s, uint32_t ms) {
  testClock += ms;
  s.update(0, false, false);
}

void holdWager(SlotMachine &s, uint32_t ms, bool increase, bool decrease) {
  testClock += ms;
  s.update(0, increase, decrease);
}

int main() {
  std::srand(7);
  SlotMachine s;
  s.start(false);
  assert(s.cash == 10000 && s.wager == 100 && !s.over);

  // GPIO13 raises the wager; GPIO14 lowers it with a held-button repeat rate.
  s.update(0, true, false); assert(s.wager == 200);
  holdWager(s, 160, true, false); assert(s.wager == 300);
  holdWager(s, 160, false, true); assert(s.wager == 200);
  holdWager(s, 160, false, true); assert(s.wager == 100);
  s.wager = 5000; holdWager(s, 160, true, false);
  assert(s.wager == 5100 && s.wagerRepeatMs < 160);
  s.update(0, false, false); assert(s.wagerRepeatMs == 160);
  s.wager = 10000; holdWager(s, 160, true, false); assert(s.wager == 10000);

  // A centered stick arms one pull; joystick-down starts the lever animation.
  s.update(0, false, false);
  s.update(-1000, false, false);
  assert(s.phase == SlotMachine::LEVER && s.cash == 0);
  advance(s, 240); assert(s.phase == SlotMachine::SPIN);
  // A triple seven is the jackpot and raises the run's peak score.
  advance(s, 1199); s.reels[0] = s.reels[1] = s.reels[2] = 5;
  advance(s, 1); assert(s.phase == SlotMachine::RESULT && s.resultKind == 2);
  assert(s.cash == 500000 && s.score == 500000);
  advance(s, 1400); assert(s.phase == SlotMachine::IDLE && !s.over);

  // A pair is a normal hit, not a triple result.
  s.cash = 10000; s.wager = 100; s.reels[0] = 3; s.reels[1] = 1; s.reels[2] = 1;
  s.update(0, false, false); s.update(-1000, false, false);
  advance(s, 240); advance(s, 1199); s.reels[0] = 3; s.reels[1] = 1; s.reels[2] = 1;
  advance(s, 1); assert(s.phase == SlotMachine::RESULT && s.resultKind == 1 && s.cash == 10100);
  advance(s, 1400); assert(s.phase == SlotMachine::IDLE && !s.over);

  // The machine permits zero cash but ends only after a spin leaves cash negative.
  s.cash = 0; s.wager = 100; s.reels[0] = 0; s.reels[1] = 1; s.reels[2] = 2;
  s.update(0, false, false); s.update(-1000, false, false);
  advance(s, 240); advance(s, 1199); s.reels[0] = 0; s.reels[1] = 1; s.reels[2] = 2;
  advance(s, 1); assert(s.phase == SlotMachine::RESULT && s.cash == -100);
  advance(s, 1400); assert(s.over);

  Adafruit_SSD1306 d; s.draw(d);
  std::cout << "PASS: Slot Machine wager controls, lever/spin/result animation, payouts, jackpot, and debt loss.\n";
}
