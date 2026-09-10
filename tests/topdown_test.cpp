#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <cmath>
#include <iostream>
#define private public
#include "../ESP32_Snake/TopdownRPG.h"
#undef private

uint32_t testClock = 0;

void tick(TopdownRPG &g, int x = 0, int y = 0, bool attack = false, bool item = false) {
  testClock += 20;
  g.update(x, y, attack, item);
}

void clearEnemies(TopdownRPG &g) {
  for (auto &e : g.enemies) e.alive = false;
  g.enemies[5] = {250, 100, 99, 99, TopdownRPG::REWARD_NONE, true, false};
}

int main() {
  TopdownRPG g;
  g.start(false);
  assert(!g.started && g.ammo == 0 && !g.swordUnlocked && !g.bowUnlocked);
  g.update(0, 0, true, false); // GPIO13 enters the intro screen.
  assert(g.started);

  // GPIO14 picks exactly one nearby rock, then the same button throws it.
  clearEnemies(g);
  g.x = 40; g.y = 31;
  tick(g, 0, 0, false, true);
  assert(g.holdingRock && !g.rocks[0].available);
  tick(g, 0, 0, false, true);
  assert(!g.holdingRock && g.rockShot.active);
  for (int i = 0; i < 100 && g.rockShot.active; ++i) tick(g);
  assert(!g.rockShot.active && g.rocks[0].available);

  // The first reward fight changes the attack from punch to sword.
  g.enemies[0] = {48, 31, 1, 1, TopdownRPG::REWARD_SWORD, true, false};
  g.x = 40; g.y = 31; g.facingX = 1; g.facingY = 0;
  tick(g, 0, 0, true);
  assert(g.swordUnlocked && !g.enemies[0].alive && g.score == 150);

  // The bow reward replaces rock pickup and starts with five arrows.
  g.enemies[2] = {48, 31, 1, 1, TopdownRPG::REWARD_BOW, true, false};
  g.x = 40; g.y = 31; g.attackCooldown = 0;
  tick(g, 0, 0, true);
  assert(g.bowUnlocked && g.ammo == 5 && !g.holdingRock);
  tick(g, 0, 0, false, true);
  assert(g.ammo == 4);

  // A spent arrow can be recovered, and an empty quiver can be crafted at a rock.
  g.arrows[0].active = false; g.arrows[0].recoverable = true;
  g.arrows[0].x = g.x + 3; g.arrows[0].y = g.y + 3;
  tick(g, 0, 0, false, true);
  assert(g.ammo == 5 && !g.arrows[0].recoverable);
  for (auto &a : g.arrows) a = {0, 0, 0, 0, false, false};
  g.ammo = 0; g.x = g.rocks[1].x - 3; g.y = g.rocks[1].y - 3;
  tick(g, 0, 0, false, true);
  assert(g.ammo == 5 && !g.rocks[1].available && !g.holdingRock);

  // Damage has immunity and repeated updates remain finite and in bounds.
  g.start(true); g.update(0, 0, true, false); clearEnemies(g);
  g.enemies[0] = {g.x, g.y, 2, 2, TopdownRPG::REWARD_NONE, true, false};
  Adafruit_SSD1306 screen;
  for (int i = 0; i < 200; ++i) {
    tick(g, (std::rand() % 3 - 1) * 1000, (std::rand() % 3 - 1) * 1000, std::rand() % 3 == 0, std::rand() % 4 == 0);
    g.draw(screen);
    assert(std::isfinite(g.x) && std::isfinite(g.y));
    assert(g.x >= 5 && g.x <= 244 && g.y >= 12 && g.y <= 100);
    if (g.over) { g.start(true); g.update(0, 0, true, false); clearEnemies(g); }
  }
  std::cout << "PASS: intro, punch/sword reward, single-rock pickup/throw, bow replacement, arrow recovery/crafting, damage immunity, bounds.\n";
}
