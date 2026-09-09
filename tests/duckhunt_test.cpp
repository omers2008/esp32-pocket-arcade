#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/DuckHunt.h"
#undef private
uint32_t testClock = 0;
void tick(DuckHunt &g, int x = 0, int y = 0, bool fire = false, uint32_t elapsed = 20) {
  testClock += elapsed; g.update(x, y, fire);
}
void miss(DuckHunt &g) { g.aimX = 64; g.aimY = 49; g.duckX = 15; g.duckY = 20; tick(g, 0, 0, true, 1); }
void hit(DuckHunt &g) { g.aimX = g.duckX; g.aimY = g.duckY; tick(g, 0, 0, true, 1); }
int main() {
  std::srand(42);
  DuckHunt g;
  for (bool hard : {false, true}) {
    g.start(hard);
    assert(g.lives == (hard ? 3 : 5) && g.ammo == 3 && !g.over);
    assert(abs(g.vx) == (hard ? 1.0f : 0.65f));
    float ax = g.aimX, ay = g.aimY;
    tick(g, 650, -650); assert(g.aimX == ax && g.aimY == ay);
    tick(g, 1800, 1800); assert(g.aimX > ax && g.aimY < ay);
    for (int i = 0; i < 150; ++i) tick(g, -4095, 4095);
    assert(g.aimX == 3 && g.aimY == 14);
    g.start(hard);
    for (int i = 0; i < 150; ++i) tick(g, 4095, -4095);
    assert(g.aimX == 124 && g.aimY == 49);
    for (int misses = 0; misses < 3; ++misses) {
      g.start(hard);
      for (int i = 0; i < misses; ++i) miss(g);
      hit(g); // Shot edges are resolved even before the next 20 ms frame.
      assert(g.phase == DuckHunt::HIT && g.score == 150 - 25 * misses);
      assert(g.lives == (hard ? 3 : 5));
      tick(g, 0, 0, true, 650); // Result-screen shot must not hit the next duck.
      assert(g.phase == DuckHunt::FLYING && g.ammo == 3);
    }
    g.start(hard);
    int allowedEscapes = g.lives;
    for (int life = allowedEscapes; life > 0; --life) {
      miss(g); miss(g); miss(g);
      assert(g.ammo == 0 && g.lives == life - 1 && g.phase == DuckHunt::ESCAPED);
      tick(g, 0, 0, true, 650);
    }
    assert(g.over && g.lives == 0 && g.score == 0);
    tick(g, 0, 0, true); assert(g.lives == 0 && g.ammo == 0);
    g.start(hard);
    tick(g, 0, 0, false, g.timeLimit() - 1);
    assert(g.phase == DuckHunt::FLYING);
    tick(g, 0, 0, false, 1);
    assert(g.phase == DuckHunt::ESCAPED && g.lives == allowedEscapes - 1);
    tick(g, 0, 0, true, 1); assert(g.lives == allowedEscapes - 1);
    // Bounces remain inside the visible target area.
    g.start(hard); g.duckX = 118.9f; g.duckY = 45.9f; g.vx = g.vy = 1;
    tick(g); assert(g.vx < 0 && g.vy < 0 && g.duckX <= 119 && g.duckY <= 46);
    g.duckX = 8.1f; g.duckY = 17.1f; g.vx = g.vy = -1;
    tick(g); assert(g.vx > 0 && g.vy > 0 && g.duckX >= 8 && g.duckY >= 17);
    g.start(hard);
    for (int i = 0; i < 20; ++i) { hit(g); tick(g, 0, 0, false, 650); }
    assert(g.round == 3 && g.completed == 20 && g.score == 3000);
    assert(abs(abs(g.vx) - (hard ? 1.08f : 0.73f)) < 0.001f);
    Adafruit_SSD1306 d;
    for (int i = 0; i < 20000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 2048, (std::rand() % 3 - 1) * 2048, std::rand() % 20 == 0);
      assert(g.aimX >= 3 && g.aimX <= 124 && g.aimY >= 14 && g.aimY <= 49);
      assert(g.duckX >= 8 && g.duckX <= 119 && g.duckY >= 17 && g.duckY <= 46);
      assert(g.ammo >= 0 && g.ammo <= 3 && g.lives >= 0 && g.lives <= allowedEscapes);
      assert(std::isfinite(g.duckX) && std::isfinite(g.aimX));
      d.drawRect(0, 0, 1, 1, 1); g.draw(d);
      if (g.over) g.start(hard);
    }
  }
  // Easy's generous hit area doesn't carry over to Hard.
  for (bool hard : {false, true}) {
    g.start(hard); g.duckX = 50; g.duckY = 30; g.aimX = 57; g.aimY = 30;
    tick(g, 0, 0, true, 1);
    assert(g.score == (hard ? 0u : 150u));
  }
  // Timers also work across the millis() rollover.
  testClock = UINT32_MAX - 100; g.start(false);
  tick(g, 0, 0, false, 120); assert(g.phase == DuckHunt::FLYING);
  hit(g); tick(g, 0, 0, false, 650); assert(g.ammo == 3 && !g.over);
  std::cout << "PASS: Duck Hunt aiming, hitboxes, shot edges, ammo, score, timeout, lives, target bounces, rounds, restart, timer rollover, 40000 input/draw steps.\n";
}
