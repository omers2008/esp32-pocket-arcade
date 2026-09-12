#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/SkyPatrol.h"
#undef private
uint32_t testClock = 0;
void tick(SkyPatrol &g, int x = 0, int y = 0, bool boost = false, bool fire = false) {
  testClock += 20; g.update(x, y, boost, fire);
}
void quiet(SkyPatrol &g) {
  for (auto &e : g.enemies) e = {};
  for (auto &s : g.shots) s = {};
  g.spawnTimer = 10000;
}
void begin(SkyPatrol &g, bool hard = false) { g.start(hard); tick(g, 0, 0, true); tick(g); quiet(g); }
int shots(SkyPatrol &g) { int n = 0; for (auto &s : g.shots) n += s.life > 0; return n; }
int main() {
  SkyPatrol g; Adafruit_SSD1306 d;
  g.start(false); tick(g, 1000, 0, false, true); g.draw(d);
  assert(!g.started && shots(g) == 0);
  tick(g, 0, 0, true); tick(g, 0, 0, true, true);
  assert(g.started && !g.boosting && shots(g) == 0);
  tick(g); quiet(g);
  tick(g, 1000); assert(abs(g.angle) < 0.001f && abs(g.scroll - 0.85f) < 0.001f);
  tick(g, 1000, 0, true); assert(abs(g.scroll - 1.65f) < 0.001f && shots(g) == 0);
  float a = g.angle; tick(g, 0, 0); assert(g.angle == a && !g.boosting);
  // Pointing uses the absolute stick direction in all eight compass directions.
  const int axes[][2] = {{1000,0},{1000,1000},{0,1000},{-1000,1000},
                         {-1000,0},{-1000,-1000},{0,-1000},{1000,-1000}};
  for (auto &axis : axes) {
    begin(g);
    for (int i = 0; i < 32; ++i) { g.y = 28; tick(g, axis[0], axis[1]); }
    float target = atan2f(-float(axis[1]), float(axis[0]));
    assert(abs(g.wrapAngle(g.angle - target)) < 0.001f);
  }
  begin(g); g.angle = 3.1f; g.y = 28;
  tick(g, -1000, 50); assert(abs(g.wrapAngle(g.angle - 3.1f)) <= 0.121f); // Short turn across +/-pi.

  begin(g); g.update(0, 0, false, true); tick(g);
  assert(shots(g) == 1 && g.shots[0].friendly);
  tick(g, 0, 0, true, true); assert(g.boosting && shots(g) == 1);
  for (int i = 0; i < 10; ++i) tick(g, 0, 0, true, true);
  assert(shots(g) >= 2);
  // Shots follow the barrel, including backwards and vertical shooting.
  for (float angle : {0.0f, 1.5707963f, 3.1415926f, -1.5707963f}) {
    begin(g); g.angle = angle; g.shoot(38, 28, angle, true);
    assert(abs(g.shots[0].vx - cosf(angle) * 3.2f) < 0.001f);
    assert(abs(g.shots[0].vy - sinf(angle) * 3.2f) < 0.001f);
  }
  // Enemy aircraft take damage and award one kill only when destroyed.
  begin(g, true); g.scroll = 0;
  g.enemies[0] = {60, 28, 3.1415926f, 2, 1000};
  for (int hp = 1; hp >= 0; --hp) {
    g.shots[0] = {53, 28, 3.2f, 0, 65, true}; g.updateShots();
    assert(g.enemies[0].hp == hp && !g.shots[0].life);
  }
  assert(g.score == 100 && g.kills == 1); g.updateShots(); assert(g.score == 100);
  g.kills = 10; assert(g.wave() == 3);
  // Opponents move and fire toward the player. Camera motion follows reversals.
  begin(g); g.enemies[0] = {90, 28, 3.1415926f, 1, 0};
  tick(g); assert(g.enemies[0].x < 90 && shots(g) == 1 && !g.shots[0].friendly);
  float ex = g.enemies[0].x; g.angle = 3.1415926f; tick(g);
  assert(g.scroll < 0 && g.enemies[0].x > ex);

  // Water kills even during respawn protection; the ceiling is a safe bound.
  begin(g); g.y = 52.9f; g.angle = 1.5707963f; tick(g, 0, -1000, true);
  assert(g.lives == 2 && g.y == 28 && g.angle == 0 && g.immune == 100);
  assert(shots(g) == 0);
  g.lives = 1; g.y = 53; tick(g); assert(g.over && g.lives == 0);
  begin(g); g.y = 14; g.angle = -1.5707963f; tick(g, 0, 1000, true);
  assert(g.y == 14 && g.lives == 3);
  // Bullet immunity, normal damage, and collisions with aircraft.
  begin(g); g.scroll = 0;
  g.shots[0] = {33, 28, 2, 0, 65, false}; g.updateShots(); assert(g.lives == 3);
  g.immune = 0; g.shots[0] = {33, 28, 2, 0, 65, false};
  assert(g.updateShots() && g.lives == 2);
  g.immune = 0; g.enemies[0] = {38, 28, 0, 1, 100}; tick(g); assert(g.lives == 1);
  begin(g); g.scroll = 0; g.shots[0] = {80, 55, 0, 3, 65, true};
  g.updateShots(); assert(!g.shots[0].life);

  std::srand(28);
  for (int hard = 0; hard < 2; ++hard) {
    testClock = UINT32_MAX - 30; begin(g, hard); g.spawnTimer = 0;
    int sawEnemies = 0;
    for (int i = 0; i < 20000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 1000, (std::rand() % 3 - 1) * 1000,
           std::rand() % 2, std::rand() % 3 == 0);
      g.draw(d); assert(std::isfinite(g.angle) && std::isfinite(g.y));
      if (!g.over) assert(g.y >= 14 && g.y < 53);
      assert(g.lives >= 0 && g.lives <= (hard ? 2 : 3));
      assert(g.scenery >= 0 && g.scenery < 192);
      for (auto &e : g.enemies) if (e.hp) { ++sawEnemies; assert(e.y >= 16 && e.y <= 48); }
      if (g.over) { begin(g, hard); g.spawnTimer = 0; }
    }
    assert(sawEnemies > 0);
  }
  g.start(false); assert(g.kills == 0 && g.score == 0 && !g.started && !g.over);
  std::cout << "PASS: absolute steering, boost/fire mapping, shot queue, camera reversal, aircraft combat, water/ceiling, damage, spawning, rollover, 40000 frames.\n";
}
