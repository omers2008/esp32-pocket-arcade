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
// Keep single-bird fixtures deterministic; flock coverage below uses real spawns.
void single(DuckHunt &g) {
  g.birdCount = g.birdsLeft = 1;
  g.birds[0].alive = true;
  g.birds[1].alive = g.birds[2].alive = false;
}
void miss(DuckHunt &g) { g.aimX = 64; g.aimY = 49; g.birds[0].x = 15; g.birds[0].y = 20; tick(g, 0, 0, true, 1); }
void hit(DuckHunt &g) { g.aimX = g.birds[0].x; g.aimY = g.birds[0].y; tick(g, 0, 0, true, 1); }
int main() {
  std::srand(42);
  DuckHunt g;
  for (bool hard : {false, true}) {
    g.start(hard); single(g);
    assert(g.lives == (hard ? 3 : 5) && g.ammo == 3 && !g.over);
    assert(abs(g.birds[0].vx) == (hard ? 1.0f : 0.65f));
    float ax = g.aimX, ay = g.aimY;
    tick(g, 650, -650); assert(g.aimX == ax && g.aimY == ay);
    tick(g, 1800, 1800); assert(g.aimX > ax && g.aimY < ay);
    for (int i = 0; i < 150; ++i) tick(g, -4095, 4095);
    assert(g.aimX == 3 && g.aimY == 14);
    g.start(hard); single(g);
    for (int i = 0; i < 150; ++i) tick(g, 4095, -4095);
    assert(g.aimX == 124 && g.aimY == 49);
    for (int misses = 0; misses < 3; ++misses) {
      g.start(hard); single(g);
      for (int i = 0; i < misses; ++i) miss(g);
      hit(g); // Shot edges are resolved even before the next 20 ms frame.
      assert(g.phase == DuckHunt::HIT && g.score == 150 - 25 * misses);
      assert(g.lives == (hard ? 3 : 5));
      tick(g, 0, 0, true, 650); // Result-screen shot must not hit the next duck.
      assert(g.phase == DuckHunt::FLYING && g.ammo == 3);
    }
    g.start(hard); single(g);
    int allowedEscapes = g.lives;
    for (int life = allowedEscapes; life > 0; --life) {
      miss(g); miss(g); miss(g);
      assert(g.ammo == 0 && g.lives == life - 1 && g.phase == DuckHunt::ESCAPED);
      tick(g, 0, 0, true, 650);
    }
    assert(g.over && g.lives == 0 && g.score == 0);
    tick(g, 0, 0, true); assert(g.lives == 0 && g.ammo == 0);
    g.start(hard); single(g);
    tick(g, 0, 0, false, g.timeLimit() - 1);
    assert(g.phase == DuckHunt::FLYING);
    tick(g, 0, 0, false, 1);
    assert(g.phase == DuckHunt::ESCAPED && g.lives == allowedEscapes - 1);
    tick(g, 0, 0, true, 1); assert(g.lives == allowedEscapes - 1);
    // Bounces remain inside the visible target area.
    g.start(hard); single(g); g.birds[0].x = 118.9f; g.birds[0].y = 45.9f; g.birds[0].vx = g.birds[0].vy = 1;
    tick(g); assert(g.birds[0].vx < 0 && g.birds[0].vy < 0 && g.birds[0].x <= 119 && g.birds[0].y <= 46);
    g.birds[0].x = 8.1f; g.birds[0].y = 17.1f; g.birds[0].vx = g.birds[0].vy = -1;
    tick(g); assert(g.birds[0].vx > 0 && g.birds[0].vy > 0 && g.birds[0].x >= 8 && g.birds[0].y >= 17);
    g.start(hard); single(g);
    for (int i = 0; i < 20; ++i) { single(g); hit(g); tick(g, 0, 0, false, 650); }
    assert(g.round == 3 && g.completed == 20 && g.score == 3000);
    assert(abs(abs(g.birds[0].vx) - (hard ? 1.08f : 0.73f)) < 0.001f);
    Adafruit_SSD1306 d;
    for (int i = 0; i < 20000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 2048, (std::rand() % 3 - 1) * 2048, std::rand() % 20 == 0);
      assert(g.aimX >= 3 && g.aimX <= 124 && g.aimY >= 14 && g.aimY <= 49);
      assert(g.birds[0].x >= 8 && g.birds[0].x <= 119 && g.birds[0].y >= 17 && g.birds[0].y <= 46);
      assert(g.ammo >= 0 && g.ammo <= 3 && g.lives >= 0 && g.lives <= allowedEscapes);
      assert(std::isfinite(g.birds[0].x) && std::isfinite(g.aimX));
      int alive = 0;
      for (int n = 0; n < g.birdCount; ++n) {
        const auto &b = g.birds[n];
        assert(b.x >= 8 && b.x <= 119 && b.y >= 17 && b.y <= 46);
        if (b.alive) ++alive;
      }
      assert(alive == g.birdsLeft);
      g.draw(d);
      if (g.over) g.start(hard);
    }
  }
  // Spawn frequencies include singles, pairs and occasional triples in both modes.
  for (bool hard : {false, true}) {
    int sizes[4] = {};
    for (int i = 0; i < 2000; ++i) {
      g.start(hard); ++sizes[g.birdCount];
      assert(g.ammo == 3 && g.birdsLeft == g.birdCount);
      assert(g.timeLimit() == (hard ? 4500u : 6500u) + (g.birdCount - 1) * 1500u);
    }
    assert(sizes[1] > sizes[2] && sizes[2] > sizes[3] && sizes[3] > 50);
    // Place a spread-out trio; each shot leaves survivors active until all are hit.
    g.start(hard); g.birdCount = g.birdsLeft = 3;
    for (int i = 0; i < 3; ++i) g.birds[i] = {float(20 + i * 35), 30, 1, 0, true, 0};
    for (int i = 0; i < 3; ++i) {
      g.aimX = g.birds[i].x; g.aimY = 30; tick(g, 0, 0, true, 1);
      assert(g.ammo == 2 - i && g.birdsLeft == 2 - i);
      assert(g.phase == (i < 2 ? DuckHunt::FLYING : DuckHunt::HIT));
    }
    assert(g.score == 375 && g.lives == (hard ? 3 : 5));
    // Overlapping birds can all be hit by one shot; dead birds cannot score twice.
    g.start(hard); g.birdCount = g.birdsLeft = 3;
    for (auto &b : g.birds) b = {50, 30, 1, 0, true, 0};
    g.birds[2].x = 95;
    g.aimX = 50; g.aimY = 30; tick(g, 0, 0, true, 1);
    assert(g.score == 300 && g.birdsLeft == 1 && g.ammo == 2);
    tick(g, 0, 0, true, 1); assert(g.score == 300 && g.ammo == 1);
    g.aimX = g.birds[2].x; tick(g, 0, 0, true, 1);
    assert(g.score == 400 && g.phase == DuckHunt::HIT);
    g.start(hard); g.birdCount = g.birdsLeft = 3;
    for (auto &b : g.birds) b = {50, 30, 1, 0, true, 0};
    g.aimX = 50; g.aimY = 30; tick(g, 0, 0, true, 1);
    assert(g.score == 450 && g.birdsLeft == 0 && g.ammo == 2 && g.phase == DuckHunt::HIT);
    // A failed flock costs one life total and retains points from birds already hit.
    g.start(hard); g.birdCount = g.birdsLeft = 3;
    for (int i = 0; i < 3; ++i) g.birds[i] = {float(20 + i * 35), 30, 1, 0, true, 0};
    hit(g);
    tick(g, 0, 0, false, g.timeLimit());
    assert(g.score == 150 && g.lives == (hard ? 2 : 4) && g.phase == DuckHunt::ESCAPED);
    tick(g, 0, 0, false, 650); assert(g.completed == 1 && g.ammo == 3);
  }
  // Easy's generous hit area doesn't carry over to Hard.
  for (bool hard : {false, true}) {
    g.start(hard); single(g); g.birds[0].x = 50; g.birds[0].y = 30; g.aimX = 57; g.aimY = 30;
    tick(g, 0, 0, true, 1);
    assert(g.score == (hard ? 0u : 150u));
  }
  // Timers also work across the millis() rollover.
  testClock = UINT32_MAX - 100; g.start(false); single(g);
  tick(g, 0, 0, false, 120); assert(g.phase == DuckHunt::FLYING);
  hit(g); tick(g, 0, 0, false, 650); assert(g.ammo == 3 && !g.over);
  std::cout << "PASS: Duck Hunt single/flock spawns, multi-hit shots, no duplicate scoring, partial flocks, aiming, ammo, timers, lives, rounds, 40000 input/draw steps.\n";
}
