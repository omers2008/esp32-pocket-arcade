#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/StreetFighter.h"
#undef private
uint32_t testClock = 0;
void tick(StreetFighter &g, int x = 0, int y = 0, bool punch = false, bool kick = false, uint32_t ms = 20) {
  testClock += ms; g.update(x, y, punch, kick);
}
void ready(StreetFighter &g, bool hard = false) {
  g.start(hard); testClock += 800; g.lastSpawn = testClock;
}
void enemy(StreetFighter &g, float x, int hp = 8, int slot = 0) {
  auto &e = g.enemies[slot]; e = StreetFighter::Enemy{};
  e.x = x; e.hp = hp; e.active = true; e.stun = 100;
}
int main() {
  std::srand(42);
  StreetFighter g;
  g.start(false); tick(g, 1000, 1000, true, true, 700);
  assert(g.x == 60 && g.y == 46 && !g.attackTicks);
  tick(g, 0, 1000, false, false, 100); assert(g.grounded); // Held menu-up cannot jump.
  tick(g); tick(g, 0, 1000); assert(!g.grounded && g.y < 46);
  float vy = g.vy; tick(g); tick(g, 0, 1000); assert(g.vy > vy); // No double jump.
  for (int i = 0; i < 40; ++i) tick(g, 0, 1000);
  assert(g.grounded && g.y == 46); // Holding up does not bunny-hop.
  tick(g); tick(g, 0, 1000); assert(!g.grounded);
  ready(g); g.lastFrame = testClock;
  g.update(0, 0, true, false); assert(g.punchQueued && !g.attackTicks);
  tick(g); assert(g.attackTicks == 6 && g.cooldown == 12); // Buffered short press.
  ready(g); enemy(g, 72);
  tick(g, 0, 0, true); assert(g.enemies[0].hp == 7);
  for (int i = 0; i < 5; ++i) tick(g);
  assert(g.enemies[0].hp == 7); // Each swing damages each enemy only once.
  tick(g, 0, 0, true); assert(g.enemies[0].hp == 7); // Cooldown blocks new attack.
  ready(g); enemy(g, 78);
  tick(g, 0, 0, true); assert(g.enemies[0].hp == 8); // Out of punch range.
  ready(g); enemy(g, 78);
  tick(g, 0, 0, false, true); assert(g.enemies[0].hp == 6 && g.cooldown == 24);
  ready(g); enemy(g, 72, 2); enemy(g, 74, 2, 1);
  tick(g, 0, 0, true, true); // Kick wins simultaneous input and can hit a group.
  assert(g.kicking && g.score == 200 && g.liveEnemies() == 0);
  ready(g); g.facing = -1; enemy(g, 49);
  tick(g, 0, 0, true); assert(g.enemies[0].hp == 7);
  for (bool hard : {false, true}) {
    ready(g, hard); int initialHp = g.hp;
    enemy(g, 70); auto &e = g.enemies[0]; e.stun = 0; e.facing = -1; e.windup = 1;
    tick(g); assert(g.hp == initialHp - 1 && g.immune > 0);
    e.x = g.x + 10; e.strike = 0; e.windup = 1;
    tick(g); assert(g.hp == initialHp - 1); // Invulnerability prevents overlapping hits.
    ready(g, hard); g.y = 20; g.vy = 0; g.grounded = false;
    enemy(g, 70); g.enemies[0].stun = 0; g.enemies[0].facing = -1; g.enemies[0].windup = 1;
    tick(g); assert(g.hp == initialHp); // Jump clears the enemy's strike height.
    ready(g, hard); g.hp = 1;
    enemy(g, 70); g.enemies[0].stun = 0; g.enemies[0].facing = -1; g.enemies[0].windup = 1;
    tick(g); assert(g.over && g.hp == 0);
    tick(g, 1000, 1000, true, true); assert(g.hp == 0); // Terminal state stays terminal.
    ready(g, hard); g.hp = initialHp - 2; g.toSpawn = 0;
    tick(g); assert(g.wave == 2 && g.score == 250 && g.hp == initialHp - 1 && g.toSpawn == 5);
    assert(g.grounded && !g.punchQueued && !g.kickQueued && !g.attackTicks);
    g.wave = 3; g.startWave(); g.x = 2; g.spawnEnemy(0); assert(g.enemies[0].x == 118);
    g.x = 118; g.spawnEnemy(1); assert(g.enemies[1].x == 2 && g.enemies[1].type == 1);
    g.spawnEnemy(2); assert(g.enemies[2].type == 2 && g.enemies[2].hp == (hard ? 5 : 4));
    // Spawn slots don't allow an old punch to damage a newly created fighter.
    g.attackTicks = 2; g.spawnEnemy(0); assert(g.enemies[0].hit);
    Adafruit_SSD1306 display;
    g.start(hard);
    for (int i = 0; i < 30000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 1000, (std::rand() % 3 - 1) * 1000,
           std::rand() % 12 == 0, std::rand() % 20 == 0);
      assert(g.x >= 2 && g.x <= 118 && g.y >= 9 && g.y <= 46);
      assert(g.hp >= 0 && g.hp <= initialHp && g.toSpawn >= 0 && g.toSpawn <= 10);
      assert(g.liveEnemies() <= (hard ? 3 : 2));
      for (const auto &foe : g.enemies) if (foe.active) assert(foe.x >= 2 && foe.x <= 118 && foe.hp > 0);
      g.draw(display);
      if (g.over) g.start(hard);
    }
  }
  testClock = UINT32_MAX - 400; g.start(false);
  tick(g, 1000, 0, false, false, 820); assert(g.x > 60 && !g.over);
  std::cout << "PASS: Fighter input queue, jump/rearm, punch/kick ranges and cooldowns, multi-hit, damage, immunity, waves, spawning, rollover, 60000 input/draw steps.\n";
}
