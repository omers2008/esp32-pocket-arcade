#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/Pinball.h"
#undef private

uint32_t testClock = 0;

int main() {
  Pinball p;
  p.start(false); assert(p.lives == 3 && p.waiting && !p.over);
  p.update(false, false); assert(p.launchReady);
  p.update(true, false); assert(!p.waiting && p.ballVy < 0);

  p.ballX = 35; p.ballY = 25; p.ballVx = 0; p.ballVy = 1;
  p.stepBall(); assert(p.score >= 25);
  p.leftHeld = true; p.ballX = 32; p.ballY = 52; p.ballVx = 0; p.ballVy = 1;
  uint32_t oldScore = p.score; p.stepBall(); assert(p.score > oldScore && p.ballVy < 0);

  p.ballX = 64; p.ballY = 65; p.ballVy = 1; p.stepBall(); assert(p.lives == 2 && p.waiting && !p.over);
  p.update(false, false); p.update(false, true); assert(!p.waiting && p.ballVy < 0);
  p.lives = 1; p.ballX = 64; p.ballY = 65; p.ballVy = 1; p.stepBall(); assert(p.over && !p.won);

  p.start(true); assert(p.lives == 2);
  Adafruit_SSD1306 d; p.draw(d);
  std::cout << "PASS: Pinball launch, flippers, bumpers, scoring, drains, lives, and difficulty.\n";
}
