#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <cmath>
#include <iostream>
#define private public
#include "../ESP32_Snake/Castle.h"
#undef private
uint32_t testClock = 0;
void tick(CastleGame &g, int x = 0, bool attack = false, bool jump = false) {
  testClock += 20; g.update(x, attack, jump);
}
void quiet(CastleGame &g) { for (auto &e : g.enemies) e.alive = false; }
int main() {
  CastleGame g; g.start(false); quiet(g);
  tick(g, 0, false, true); assert(g.y < 49 && !g.grounded);
  float before = g.vy; tick(g, 0, false, true); assert(g.vy > before); // no double jump
  for (int i = 0; i < 40; ++i) tick(g);
  assert(g.grounded && g.y == 49);
  // Event queued between render/physics ticks.
  g.update(0, false, true); tick(g); assert(g.y < 49);
  // Both pits can be crossed in both difficulties and all stages.
  for (int hard = 0; hard < 2; ++hard) for (int stage = 1; stage <= 3; ++stage) {
    for (int pit = 0; pit < 2; ++pit) {
      g.start(hard); g.stage = stage; g.loadStage(); quiet(g);
      int left = pit == 0 ? 160 + (stage - 1) * 6 : 350 - (stage - 1) * 6;
      int right = pit == 0 ? left + 24 : left + 28;
      g.x = left - 12; g.checkpoint = g.x;
      tick(g, 1000, false, true);
      for (int i = 0; i < 30; ++i) tick(g, 1000);
      assert(!g.over && g.x > right && g.hp == (hard ? 4 : 6));
    }
  }
  // Landing on a one-way platform.
  g.start(false); quiet(g); g.x = 110;
  tick(g, 0, false, true);
  for (int i = 0; i < 35; ++i) tick(g);
  assert(g.grounded && g.y == 32);
  for (int lower : {1, 3}) {
    g.start(false); quiet(g);
    g.x = float(g.platforms[lower].x + g.platforms[lower].w - 1);
    g.y = 32;
    tick(g, 1000, false, true);
    for (int i = 0; i < 25; ++i) tick(g, 1000);
    for (int i = 0; i < 10; ++i) tick(g);
    assert(g.grounded && g.y == 20); // Both upper candle platforms reachable.
  }
  // One enemy hit per swing; held attack repeats after cooldown.
  g.start(true); quiet(g); g.x = 70;
  g.enemies[0] = {84, 51, 84, 1, 2, 0, false, true, false};
  tick(g, 0, true); assert(g.enemies[0].hp == 1);
  for (int i = 0; i < 8; ++i) tick(g, 0, true);
  assert(g.enemies[0].hp == 1);
  g.enemies[0].x = g.enemies[0].origin = 84;
  for (int i = 0; i < 12; ++i) tick(g, 0, true);
  assert(!g.enemies[0].alive && g.score == 100);
  // Left-facing attack and candle healing.
  g.start(false); quiet(g); g.x = 120; g.y = 32; g.facing = -1; g.hp = 3;
  tick(g, 0, true); assert(!g.candles[0].alive && g.hp == 4 && g.score == 50);
  // Contact immunity and fall respawn / loss.
  g.start(false); quiet(g); g.immune = 0; g.damage(); g.damage(); assert(g.hp == 5);
  g.y = 71; tick(g); assert(g.hp == 3 && g.x == g.checkpoint && g.y == 49);
  g.hp = 2; g.y = 71; tick(g); assert(g.over && g.hp == 0);
  // Boss hit, victory gate, level reset and complete campaign.
  g.start(false); quiet(g); g.x = 620;
  tick(g); assert(g.stage == 1 && !g.over); // door locked
  g.x = 560; g.bossX = 580; g.bossHp = 1;
  tick(g, 0, true); assert(g.bossHp == 0 && g.score == 500);
  for (int stage = 1; stage <= 3; ++stage) {
    g.bossHp = 0; g.x = 620; tick(g);
    if (stage < 3) { assert(g.stage == stage + 1 && !g.over && g.x == 10); quiet(g); }
  }
  assert(g.over && g.won && g.score == 1400);
  Adafruit_SSD1306 screen;
  for (int hard = 0; hard < 2; ++hard) {
    g.start(hard);
    for (int i = 0; i < 20000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 1000, std::rand() % 2, std::rand() % 15 == 0);
      assert(std::isfinite(g.x) && std::isfinite(g.y));
      assert(g.x >= 0 && g.x <= 632 && g.hp >= 0 && g.hp <= (hard ? 4 : 6));
      assert(g.stage >= 1 && g.stage <= 3);
      g.draw(screen);
      if (g.over) g.start(hard);
    }
  }
  std::cout << "PASS: jump, event queue, no double jump, all pits, platforms, whip, hold-repeat, healing, immunity, respawn, boss, stage/win flow, 40000 input/draw steps.\n";
}
