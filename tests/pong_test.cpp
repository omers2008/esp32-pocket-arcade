#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/Pong.h"
#undef private
uint32_t testClock = 0;
void tick(Pong &g, int stick = 0, bool serve = false) {
  testClock += 20; g.update(stick, serve);
}
bool near(float a, float b) { return abs(a - b) < 0.001f; }
int main() {
  std::srand(42);
  for (bool hard : {false, true}) {
    Pong g; g.start(hard);
    g.update(0, true); tick(g); // Queue the serve between physics frames.
    assert(!g.waiting && near(g.speed, hard ? 2.3f : 1.6f));
    assert(g.vx < 0);
    g.cpuTarget = 55; g.lastAim = testClock;
    tick(g);
    assert(near(g.cpuY, 36 + (hard ? 2.0f : 1.65f)));
    for (int i = 0; i < 3; ++i) tick(g);
    assert(g.cpuTarget == 55); // No retargeting before 100 ms.
    tick(g); assert(g.cpuTarget == 36); // Ball is moving toward the player.
    for (int i = 0; i < 100; ++i) {
      g.ballX = 70; g.ballY = 40; g.vx = 2; g.vy = 0.5f;
      g.lastAim = testClock - 100;
      tick(g);
      assert(abs(g.aimError) <= (hard ? 4 : 2));
      assert(near(g.cpuTarget, (hard ? 52.5f : 40.0f) + g.aimError));
    }
    g.ballX = 54; g.vx = 2; g.lastAim = testClock - 100;
    tick(g);
    if (hard) assert(g.cpuTarget == 36); // Later predictive tracking threshold.
    g.speed = 10; g.bounce(36, 1);
    assert(near(g.speed, hard ? 3.8f : 3.1f)); // Ball speed cap unchanged.
    g.playerScore = 6; g.ballX = 130; g.vx = 1; tick(g);
    assert(g.over && g.playerScore == 7);
    g.start(hard); assert(!g.over && g.bestRally == 0 && g.waiting);
    Adafruit_SSD1306 d;
    for (int i = 0; i < 20000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 1000, true);
      assert(g.cpuY >= 17 && g.cpuY <= 56 && g.playerY >= 17 && g.playerY <= 56);
      assert(std::isfinite(g.ballX) && std::isfinite(g.ballY));
      assert(g.playerScore <= 7 && g.cpuScore <= 7);
      g.draw(d);
      if (g.over) g.start(hard);
    }
  }
  std::cout << "PASS: Pong CPU speed/reaction/accuracy, prediction threshold, serve queue, ball caps, scoring, restart, 40000 input/draw steps.\n";
}
