#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/TempleQuest.h"
#undef private
uint32_t testClock = 0;
void tick(TempleQuest &g, int x = 0, int y = 0, bool fire = false, bool jump = false, uint32_t ms = 20) {
  testClock += ms; g.update(x, y, fire, jump);
}
void quiet(TempleQuest &g) { for (auto &room : g.enemies) for (auto &e : room) e.alive = false; }
void ready(TempleQuest &g, bool hard = false) { g.start(hard); tick(g, 0, 0, true); quiet(g); }
int main() {
  TempleQuest g;
  g.start(false); tick(g, 1000, 0, false, true); assert(!g.started && g.x == 8);
  tick(g, 0, 0, true); assert(g.started && g.knives == 6); // Intro press is not a dagger.
  quiet(g);
  tick(g, 0, 0, false, true); assert(g.y < 48 && !g.grounded);
  float vy = g.vy; tick(g, 0, 0, false, true); assert(g.vy > vy); // No double jump.
  for (int i = 0; i < 40; ++i) tick(g);
  assert(g.grounded); // Land on the upper ledge or floor.
  // Every ledge/gem can be reached by its ladder, including both full-height shafts.
  for (int r = 0; r < 6; ++r) {
    ready(g); int ladder = TempleQuest::ladderX(r);
    g.enterRoom(r, ladder - 3, 48, false);
    for (int i = 0; i < 24; ++i) tick(g, 0, 1000);
    assert(g.room == r && g.y == 24);
    for (int i = 0; i < 20 && !g.gemTaken[r]; ++i) {
      int direction = g.x + 3 < TempleQuest::gemX(r) + 2 ? 1000 : -1000;
      tick(g, direction);
    }
    assert(g.gemTaken[r] && g.gems == 1 && g.score == 100 && g.knives == 7);
    for (int i = 0; i < 10; ++i) tick(g);
    assert(g.score == 100 && g.gems == 1); // No duplicate pickup scoring.
  }
  // Shaft connections and key-locked lower-right chamber.
  ready(g); g.x = 25;
  tick(g, 0, -1000); assert(g.room == 3 && g.climbing && g.y == 9);
  tick(g, 0, 1000); assert(g.room == 0 && g.y == 48);
  g.enterRoom(2, 93, 48, false);
  tick(g, 0, -1000); assert(g.room == 2 && g.message == 2);
  g.keys = 2; tick(g, 0, -1000); assert(g.room == 5 && g.unlocked == 2 && g.keys == 0);
  tick(g, 0, 1000); assert(g.room == 2); // Opened gate remains open.
  // Horizontal locked doors consume only their matching key.
  ready(g); g.x = 122; tick(g, 1000); assert(g.room == 0 && g.message == 1);
  g.keys = 2; tick(g, 1000); assert(g.room == 0 && g.keys == 2);
  g.keys |= 1; tick(g, 1000); assert(g.room == 1 && g.keys == 2 && (g.unlocked & 1));
  for (int i = 0; i < 5 && g.room == 1; ++i) tick(g, -1000);
  assert(g.room == 0);
  // Jump clearance over every pit and spike strip in both difficulties.
  for (bool hard : {false, true}) {
    for (int r : {1, 3, 4}) {
      ready(g, hard); int left = r == 1 ? 60 : 64;
      g.enterRoom(r, left - 12, 48, false);
      tick(g, 1000, 0, false, true);
      for (int i = 0; i < 29; ++i) tick(g, 1000);
      assert(g.lives == (hard ? 3 : 5) && g.x > left + 18 && g.grounded);
      g.enterRoom(r, left + 27, 48, false);
      tick(g, -1000, 0, false, true);
      for (int i = 0; i < 29; ++i) tick(g, -1000);
      assert(g.lives == (hard ? 3 : 5) && g.x + 6 < left && g.grounded);
    }
    for (int r : {2, 5}) {
      ready(g, hard); g.enterRoom(r, 44, 48, false);
      tick(g, 1000, 0, false, true);
      for (int i = 0; i < 28; ++i) tick(g, 1000);
      assert(g.lives == (hard ? 3 : 5) && g.x > 67);
      g.enterRoom(r, 76, 48, false);
      tick(g, -1000, 0, false, true);
      for (int i = 0; i < 28; ++i) tick(g, -1000);
      assert(g.lives == (hard ? 3 : 5) && g.x + 6 < 58);
    }
  }
  // A thrown dagger kills once, and ammunition is bounded by the two slots.
  ready(g); g.x = 40; g.facing = 1;
  g.enemies[0][0] = {50, 52, 50, 50, 1, 0, false, true};
  tick(g, 0, 0, true);
  for (int i = 0; i < 4; ++i) tick(g);
  assert(!g.enemies[0][0].alive && g.knives == 5 && g.score == 50);
  ready(g); tick(g, 0, 0, true); tick(g, 0, 0, true); tick(g, 0, 0, true);
  assert(g.knives == 4); // Third shot cannot consume ammo when both slots are full.
  g.knives = 0; tick(g, 0, 0, true); assert(g.knives == 0);
  ready(g); g.lastFrame = testClock;
  g.update(0, 0, true, true); tick(g); assert(g.knives == 5 && !g.grounded);
  // Death restores the entry checkpoint but keeps collected items and dead enemies.
  ready(g); g.enterRoom(3, 25, 9, true); g.gemTaken[3] = true; g.gems = 1;
  g.keys = g.takenKeys = 1; g.unlocked = 2;
  g.y = 70; g.climbing = false; tick(g);
  assert(g.lives == 4 && g.x == 25 && g.y == 9 && g.climbing);
  assert(g.gems == 1 && g.keys == 1 && g.unlocked == 2 && !g.enemies[3][0].alive);
  g.immune = 0; g.enemies[3][0] = {25, 9, 25, 25, 1, 0, false, true};
  tick(g); assert(g.lives == 3);
  tick(g); assert(g.lives == 3); // Respawn immunity.
  g.lives = 1; g.y = 70; g.climbing = false; tick(g); assert(g.over && g.lives == 0);
  tick(g, 1000, 1000, true, true); assert(g.lives == 0);
  // Room/key/gem progression through the connected map, plus the final idol gate.
  ready(g);
  g.enterRoom(0, 32, 24, false); tick(g); assert(g.gems == 1);
  assert(g.tryRoom(3, 25, 9, true));
  g.x = 32; g.y = 24; g.climbing = false; tick(g); assert(g.gems == 2);
  g.x = 106; g.y = 48; tick(g); assert(g.keys == 1 && g.takenKeys == 1);
  assert(g.tryRoom(4, 3, 48, false));
  g.x = 28; g.y = 24; tick(g); assert(g.gems == 3);
  assert(!g.tryRoom(5, 3, 48, false)); // Need the second key from upper middle.
  assert(g.tryRoom(3, 118, 48, false)); assert(g.tryRoom(0, 25, 48, false));
  assert(g.tryRoom(1, 3, 48, false)); assert(g.keys == 0);
  g.x = 88; g.y = 24; tick(g); g.x = 106; g.y = 24; tick(g);
  assert(g.gems == 4 && g.keys == 2 && g.takenKeys == 3);
  assert(g.tryRoom(2, 3, 48, false)); g.x = 104; g.y = 24; tick(g); assert(g.gems == 5);
  assert(g.tryRoom(5, 93, 9, true)); g.climbing = false; g.x = 112; g.y = 48; tick(g);
  assert(!g.over && g.message == 3);
  g.x = 100; g.y = 24; tick(g); assert(g.gems == 6);
  g.x = 112; g.y = 48; tick(g); assert(g.over && g.won && g.score == 1700);
  // Random input and renderer traversal preserve room, item, ammo and life bounds.
  Adafruit_SSD1306 d;
  for (bool hard : {false, true}) {
    g.start(hard);
    for (int i = 0; i < 30000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 1000, (std::rand() % 3 - 1) * 1000,
           std::rand() % 12 == 0, std::rand() % 15 == 0);
      assert(g.room >= 0 && g.room < 6 && g.gems >= 0 && g.gems <= 6);
      assert(g.knives >= 0 && g.knives <= 9 && g.lives >= 0 && g.lives <= (hard ? 3 : 5));
      assert(g.x >= 0 && g.x <= 122 && std::isfinite(g.y));
      assert((g.keys & ~3) == 0 && (g.unlocked & ~3) == 0);
      g.draw(d);
      if (g.over) g.start(hard);
    }
  }
  testClock = UINT32_MAX - 10; ready(g); tick(g, 1000); assert(g.x > 8);
  std::cout << "PASS: Temple intro, jump, ladders, all ledges/pits/spikes, room gates, keys, gems, daggers, checkpoint persistence, win/loss, 60000 input/draw steps.\n";
}
