#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/Asteroids.h"
#undef private
uint32_t testClock = 0;
void tick(Asteroids &g, int x = 0, int y = 0, bool fire = false, bool hyper = false) {
  testClock += 20; g.update(x, y, fire, hyper);
}
void begin(Asteroids &g, bool hard = false) {
  g.start(hard); tick(g, 0, 0, true); tick(g);
}
void clear(Asteroids &g) {
  for (auto &r : g.rocks) r = {};
  for (auto &s : g.shots) s = {};
}
int count(const Asteroids &g) { int n = 0; for (auto &r : g.rocks) n += r.size > 0; return n; }
int shotCount(const Asteroids &g) { int n = 0; for (auto &s : g.shots) n += s.life > 0; return n; }
int main() {
  Asteroids g; Adafruit_SSD1306 d;
  g.start(false); g.draw(d); tick(g, 1000, 1000, false, true);
  assert(!g.started && g.score == 0 && g.lives == 3);
  tick(g, 0, 0, true); tick(g, 0, 0, true);
  assert(g.started && shotCount(g) == 0); // Intro fire must be released.
  tick(g); clear(g); g.rocks[0] = {90, 15, 0, 0, 1};
  g.x = 60; g.y = 27; g.angle = 0;
  tick(g, 0, 1000); assert(g.vx > 0 && g.x > 60);
  float x = g.x, vx = g.vx; tick(g); assert(g.x > x && g.vx > 0 && g.vx < vx);
  float a = g.angle; tick(g, 1000); assert(g.angle > a);
  g.immune = 10000;
  for (int i = 0; i < 200; ++i) tick(g, 0, 1000);
  assert(g.vx * g.vx + g.vy * g.vy <= 2.89f);
  g.x = 127.8f; g.y = 53.8f; g.vx = g.vy = 0.5f;
  tick(g); assert(g.x < 1 && g.y < 1);
  assert(g.touches(127, 53, 1, 1, 3)); // Collisions across both seams.

  begin(g); clear(g); g.rocks[0] = {100, 40, 0, 0, 1};
  g.x = 40; g.y = 20; g.angle = 0;
  g.update(0, 0, true, false); tick(g); assert(shotCount(g) == 1);
  tick(g, 0, 0, true); assert(shotCount(g) == 1);
  for (int i = 0; i < 10; ++i) tick(g, 0, 0, true);
  assert(shotCount(g) >= 2);
  clear(g); g.shots[0] = {127.8f, 20, 2, 0, 10};
  g.updateShots(); assert(g.shots[0].x < 3 && g.shots[0].life == 9);
  for (int i = 0; i < 10; ++i) g.updateShots(); assert(!g.shots[0].life);

  // Real bullet impact splits one large rock into two medium rocks once.
  clear(g); g.score = 0; g.rocks[0] = {2, 20, 0.2f, 0, 3};
  g.shots[0] = {126, 20, 2, 0, 10}; g.updateShots();
  assert(!g.shots[0].life && count(g) == 2 && g.score == 20);
  assert(g.rocks[0].size == 2 && g.rocks[1].size == 2);
  g.split(0); assert(count(g) == 3 && g.score == 70);
  assert(g.rocks[0].size == 1);
  g.split(0); assert(count(g) == 2 && g.score == 170);
  g.split(0); assert(g.score == 170);

  // The worst case needs twenty simultaneous small fragments; nothing is lost.
  clear(g); g.score = 0;
  for (int i = 0; i < 5; ++i) g.rocks[i] = {float(10 + i * 20), 20, 0.3f, 0, 3};
  for (int i = 0; i < 5; ++i) g.split(i);
  assert(count(g) == 10);
  for (int i = 0; i < 24; ++i) if (g.rocks[i].size == 2) g.split(i);
  assert(count(g) == 20);
  for (int i = 0; i < 24; ++i) g.split(i);
  assert(count(g) == 0 && g.score == 2600);
  tick(g); assert(g.wave == 2 && count(g) >= 2 && g.score == 2800 && g.immune > 0);

  begin(g); clear(g); g.rocks[0] = {g.x, g.y, 0, 0, 3};
  g.immune = 0; tick(g); assert(g.lives == 2 && g.immune == 100 && !g.over);
  g.rocks[0] = {g.x, g.y, 0, 0, 3}; tick(g); assert(g.lives == 2);
  g.immune = 0; g.lives = 1; tick(g); assert(g.over && g.lives == 0);
  uint32_t score = g.score; tick(g, 1000, 1000, true, true); assert(g.score == score);

  begin(g); float oldX = g.x, oldY = g.y;
  g.update(0, 0, false, true); tick(g);
  assert(g.hyperCooldown == 200 && g.immune == 35 && g.vx == 0 && g.vy == 0);
  assert(!g.touches(g.x, g.y, oldX, oldY, 16));
  oldX = g.x; oldY = g.y; tick(g, 0, 0, false, true);
  assert(g.x == oldX && g.y == oldY && g.hyperCooldown == 199);

  std::srand(42);
  for (int hard = 0; hard < 2; ++hard) {
    testClock = UINT32_MAX - 30; begin(g, hard);
    for (int i = 0; i < 20000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 1000, std::rand() % 2 * 1000,
           std::rand() % 3 == 0, std::rand() % 50 == 0);
      g.draw(d);
      assert(g.x >= 0 && g.x < 128 && g.y >= 0 && g.y < 54);
      assert(std::isfinite(g.angle) && g.lives >= 0 && g.lives <= (hard ? 2 : 3));
      for (auto &r : g.rocks) if (r.size) {
        assert(r.size <= 3 && r.x >= 0 && r.x < 128 && r.y >= 0 && r.y < 54);
      }
      for (auto &s : g.shots) if (s.life) assert(s.x >= 0 && s.x < 128 && s.y >= 0 && s.y < 54);
      if (g.over) begin(g, hard);
    }
  }
  g.start(false); assert(g.score == 0 && g.wave == 1 && g.lives == 3 && !g.started && !g.over);
  std::cout << "PASS: rotation/thrust/inertia, wrapping and seam hits, fire queue/cooldown, splitting/capacity/scoring, waves, death/shield, hyperspace, rollover, 40000 frames.\n";
}
