#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/BattleTanks.h"
#undef private
uint32_t testClock = 0;
void tick(BattleTanks &g, int x = 0, int y = 0, bool normal = false, bool bounce = false) {
  testClock += 20; g.update(x, y, normal, bounce);
}
void begin(BattleTanks &g, bool hard = false) {
  g.start(hard); g.update(0, 0, true, false); tick(g);
  for (auto &c : g.cpus) c.cooldown = 10000;
}
void quiet(BattleTanks &g) { for (auto &c : g.cpus) c.hp = 0; }
int shots(const BattleTanks &g) {
  int n = 0; for (auto &s : g.shells) n += s.active; return n;
}
int main() {
  BattleTanks g; Adafruit_SSD1306 screen;
  g.start(false); g.draw(screen);
  tick(g, 1000, 1000, false, true);
  assert(!g.started && g.player.x == 16 && shots(g) == 0);
  tick(g, 0, 0, true); tick(g, 0, 0, true);
  assert(g.started && shots(g) == 0); // Must release intro button.
  tick(g); tick(g, 0, 1000);
  assert(g.player.x > 16 && g.player.y == 37);
  tick(g, 0, -1000); assert(g.player.x < 16.8f);
  float x = g.player.x; tick(g, 1000); assert(g.player.angle > 0 && g.player.x == x);
  tick(g, -1000); assert(abs(g.player.angle) < 0.0001f);
  g.player = {38.9f, 30, 0, 1, 0, 0};
  tick(g, 0, 1000); assert(g.player.x == 38.9f); // Solid cover.
  g.player = {4.1f, 16, BattleTanks::PI_VALUE, 1, 0, 0};
  tick(g, 0, 1000); assert(g.player.x == 4.1f); // Arena boundary.

  begin(g);
  g.update(0, 0, true, false); tick(g);
  assert(shots(g) == 1 && !g.shells[0].bouncy); // Between-frame press retained.
  tick(g, 0, 0, true); assert(shots(g) == 1); // Rate limited.
  begin(g); tick(g, 0, 0, false, true);
  assert(shots(g) == 1 && g.shells[0].bouncy);

  // Full trajectories in the clear top corridor: four reflections, then expire.
  begin(g); quiet(g); g.player = {16, 56, 0, 1, 0, 0};
  auto &s = g.shells[0];
  s = {124, 16, 2.2f, 0, 0, 900, true, true, false};
  g.updateShells(); assert(!s.active && s.bounces == 0);
  s = {124, 16, 2.2f, 0, 0, 900, true, true, true};
  int observed = 0;
  while (s.active) {
    int previous = s.bounces; float speed = abs(s.vx);
    g.updateShells(); assert(s.bounces <= 4);
    if (s.bounces > previous) { ++observed; assert(abs(s.vx) == speed && s.active); }
  }
  assert(observed == 4 && s.life > 0); // Died on impact five, not the timeout.
  s = {124, 60, 2.2f, 2.2f, 0, 900, true, true, true};
  g.updateShells(); assert(s.bounces == 1 && s.vx < 0 && s.vy < 0);
  s = {40, 30, 2.2f, 0, 0, 900, true, true, true};
  g.updateShells(); assert(s.bounces == 1 && s.vx < 0 && s.x < 42);
  s = {47, 19, 0, 2.2f, 0, 900, true, true, true};
  g.updateShells(); assert(s.bounces == 1 && s.vy < 0);

  // Tank hit consumes a shell once; only destruction awards points.
  g.cpus[0] = {25, 16, 0, 2, 1000, 0};
  for (int hp = 1; hp >= 0; --hp) {
    s = {20, 16, 2.2f, 0, 0, 900, true, true, false};
    g.updateShells(); assert(!s.active && g.cpus[0].hp == hp);
  }
  assert(g.score == 100); g.updateShells(); assert(g.score == 100);
  g.player = {16, 56, 0, 1, 0, 0}; g.immune = 0;
  s = {11, 56, 2.2f, 0, 1, 900, true, true, true};
  g.updateShells(); assert(g.hp == 4 && !s.active); // Own ricochet hurts.
  s = {11, 56, 2.2f, 0, 0, 900, true, false, false};
  g.updateShells(); assert(g.hp == 4 && !s.active); // Immunity.
  g.hp = 1; g.immune = 0;
  s = {11, 56, 2.2f, 0, 0, 900, true, false, false};
  g.updateShells(); assert(g.hp == 0 && g.over && !g.won);

  // CPU aims and fires down a clear lane, never deliberately through cover.
  begin(g); quiet(g); g.player = {16, 16, 0, 1, 0, 0};
  g.cpus[0] = {111, 16, BattleTanks::PI_VALUE, 2, 0, 0};
  g.updateCPUs(); assert(shots(g) == 1 && !g.shells[0].playerOwned);
  begin(g); quiet(g); g.player = {16, 30, 0, 1, 0, 0};
  g.cpus[0] = {64, 30, BattleTanks::PI_VALUE, 2, 0, 0};
  g.updateCPUs(); assert(shots(g) == 0);

  // Wave transition clears projectiles and preserves the score/health cap.
  begin(g); g.hp = 2;
  for (int w = 1; w <= 5; ++w) {
    quiet(g); tick(g);
    if (w < 5) { assert(g.wave == w + 1 && !g.over && shots(g) == 0); }
  }
  assert(g.over && g.won && g.score == 1500 && g.hp == 5);

  // Both difficulties, real CPU inputs/draw, restart, and millis wraparound.
  std::srand(77);
  for (int hard = 0; hard < 2; ++hard) {
    testClock = UINT32_MAX - 30; begin(g, hard);
    for (int i = 0; i < 20000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 1000, (std::rand() % 3 - 1) * 1000,
           std::rand() % 4 == 0, std::rand() % 5 == 0);
      g.draw(screen);
      assert(std::isfinite(g.player.angle) && !g.solid(g.player.x, g.player.y, 4));
      assert(g.hp >= 0 && g.hp <= (hard ? 3 : 5) && g.wave <= 5);
      for (auto &c : g.cpus) if (c.hp) assert(!g.solid(c.x, c.y, 4));
      for (auto &shell : g.shells) if (shell.active) {
        assert(shell.bounces <= 4 && shell.life > 0);
        assert(!g.solid(shell.x, shell.y, 1));
      }
      if (g.over) begin(g, hard);
    }
  }
  std::cout << "PASS: tank steering/reverse, walls, fire gating, four ricochets, corners/cover, damage/scoring, CPU aim, waves, rollover, 40000 input/draw steps.\n";
}
