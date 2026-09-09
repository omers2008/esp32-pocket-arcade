#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/RogueCards.h"
#undef private
uint32_t testClock = 0;
void tick(RogueCards &g, int x = 0, int y = 0, bool pick = false, bool end = false, uint32_t ms = 250) {
  testClock += ms; g.update(x, y, pick, end);
}
void combat(RogueCards &g) { g.start(false); tick(g, 0, 0, true); testClock += 250; }
void uniqueChoices(const RogueCards &g) {
  for (int i = 0; i < 3; ++i) {
    assert(g.choices[i] >= 0 && g.choices[i] < 6);
    for (int j = 0; j < i; ++j) assert(g.choices[i] != g.choices[j]);
  }
}
int main() {
  std::srand(42); RogueCards g;
  g.start(false); assert(g.phase == RogueCards::INTRO && g.hp == 32);
  tick(g, 0, 0, true, false, 100); assert(g.phase == RogueCards::INTRO);
  tick(g, 0, 0, true); assert(g.phase == RogueCards::COMBAT && g.energy == 3 && g.handSize == 3);
  tick(g); tick(g, 1000); assert(g.selected == 1);
  tick(g, 1000); assert(g.selected == 1); // One menu step until neutral.
  tick(g); tick(g, 1000); assert(g.selected == 2);
  // No real-time damage: combat changes only after player actions.
  int hp = g.hp, enemyHp = g.enemyHp;
  tick(g, 0, 0, false, false, 50000); assert(g.hp == hp && g.enemyHp == enemyHp);
  g.hand[0] = RogueCards::STRIKE; g.hand[1] = RogueCards::SHIELD; g.hand[2] = RogueCards::HEAL; g.selected = 0;
  g.passives[RogueCards::MIGHT] = 2; g.passives[RogueCards::GUARD] = 1; g.passives[RogueCards::HERBS] = 1;
  tick(g, 0, 0, true); assert(g.enemyHp == enemyHp - 7 && g.energy == 2 && g.handSize == 2);
  tick(g, 0, 0, true); assert(g.block == 7 && g.energy == 1 && g.handSize == 1);
  g.hp = 10; tick(g, 0, 0, true); assert(g.hp == 14 && g.energy == 0 && g.handSize == 0);
  tick(g, 0, 0, true); assert(g.hp == 14 && g.energy == 0);
  tick(g, 0, 0, true, true); assert(g.phase == RogueCards::ENEMY); // End has priority.
  tick(g, 0, 0, true, true, 599); assert(g.phase == RogueCards::ENEMY);
  tick(g, 0, 0, false, false, 1);
  assert(g.phase == RogueCards::COMBAT && g.hp == 14 && g.block == 0 && g.handSize == 3 && g.energy == 3);
  // Every starter hand comes from the defined six-card deck.
  for (int i = 0; i < 1000; ++i) {
    g.newTurn(); int counts[3] = {};
    for (int c : g.hand) { assert(c >= 0 && c < 3); ++counts[c]; }
    assert(counts[0] <= 3 && counts[1] <= 2 && counts[2] <= 1);
  }
  combat(g); g.hp = 20; g.gainPassive(RogueCards::HEART);
  assert(g.maxHp == 37 && g.hp == 25);
  g.gainPassive(RogueCards::HEART); assert(g.maxHp == 42 && g.hp == 30);
  // Actives are free, once per battle; duplicates upgrade their effects.
  for (int a = 0; a < 6; ++a) {
    combat(g); g.handSize = 0; g.actives[a] = 2; g.enemyHp = 100; g.hp = 10;
    tick(g, 0, 0, true);
    assert(g.activeUsed[a] && g.energy == 3);
    if (a == RogueCards::FIREBALL) assert(g.enemyHp == 86);
    if (a == RogueCards::WARD) assert(g.block == 14);
    if (a == RogueCards::MEND) assert(g.hp == 20);
    if (a == RogueCards::VENOM) assert(g.enemyPoison == 6);
    if (a == RogueCards::LEECH) assert(g.enemyHp == 90 && g.hp == 14);
    if (a == RogueCards::STORM) assert(g.enemyHp == 92 && g.block == 8);
    int oldHp = g.hp, oldEnemy = g.enemyHp, oldBlock = g.block;
    tick(g, 0, 0, true); assert(g.hp == oldHp && g.enemyHp == oldEnemy && g.block == oldBlock);
    g.newTurn(); assert(g.activeUsed[a]); // Does not reset each turn.
    g.beginBattle(); assert(!g.activeUsed[a] && g.actives[a] == 2);
  }
  combat(g); g.enemyHp = 3; g.enemyPoison = 4; g.hp = 1; g.enter(RogueCards::ENEMY);
  tick(g, 0, 0, false, false, 600);
  assert(g.phase == RogueCards::PASSIVE && !g.over); // Poison wins before incoming damage.
  combat(g); g.hp = 1; g.enemyHp = 1; g.passives[RogueCards::THORNS] = 1;
  g.enter(RogueCards::ENEMY); tick(g, 0, 0, false, false, 600);
  assert(g.over && !g.won && g.hp == 0); // Mutual KO is a loss.
  // All nine battle rewards, including final boss passive THEN active before winning.
  for (int floor = 1; floor <= 9; ++floor) {
    combat(g); g.battle = floor; g.victory(); uniqueChoices(g);
    assert(g.phase == RogueCards::PASSIVE && !g.over);
    tick(g, 0, 0, true);
    if (floor % 3 == 0) {
      assert(g.phase == RogueCards::ACTIVE && !g.over); uniqueChoices(g);
      int chosen = g.choices[g.selected]; tick(g, 0, 0, true); assert(g.actives[chosen] == 1);
    }
    if (floor == 9) assert(g.over && g.won && g.score == 1025);
    else assert(g.phase == RogueCards::COMBAT || g.phase == RogueCards::EVENT);
  }
  // Events grant rewards exactly once, then advance instead of rolling another event.
  for (int kind = 0; kind < 3; ++kind) {
    combat(g); g.battle = 3; g.hp = 5; g.eventKind = kind; g.enter(RogueCards::EVENT);
    tick(g, 0, 0, true);
    if (kind < 2) {
      assert(g.rewardFromEvent && g.phase == (kind == 0 ? RogueCards::PASSIVE : RogueCards::ACTIVE));
      uniqueChoices(g); tick(g, 0, 0, true);
    } else assert(g.hp == 12);
    assert(g.battle == 4 && g.phase == RogueCards::COMBAT);
  }
  int events[4] = {};
  for (int i = 0; i < 1000; ++i) {
    combat(g); g.afterRewards();
    ++events[g.phase == RogueCards::EVENT ? g.eventKind : 3];
  }
  for (int count : events) assert(count > 50);
  Adafruit_SSD1306 d;
  for (bool hard : {false, true}) {
    g.start(hard); assert(g.maxHp == (hard ? 26 : 32));
    for (int i = 0; i < 20000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 1000, (std::rand() % 3 - 1) * 1000, std::rand() % 2 == 0, std::rand() % 8 == 0);
      assert(g.hp >= 0 && g.hp <= g.maxHp && g.battle >= 1 && g.battle <= 9);
      assert(g.energy >= 0 && g.energy <= 3 && g.handSize >= 0 && g.handSize <= 3);
      assert(g.actionCount() <= 9 && g.selected >= 0);
      if (g.phase == RogueCards::PASSIVE || g.phase == RogueCards::ACTIVE) uniqueChoices(g);
      if (g.phase == RogueCards::COMBAT) assert(!g.actionCount() || g.selected < g.actionCount());
      g.draw(d);
      if (g.over) {
        g.start(hard);
        for (auto p : g.passives) assert(p == 0);
        for (auto a : g.actives) assert(a == 0);
      }
    }
  }
  testClock = UINT32_MAX - 100; g.start(false);
  tick(g, 0, 0, true, false, 250); assert(g.phase == RogueCards::COMBAT);
  // Play complete runs through the actual input/state API, without changing HP,
  // cards, or enemies. This checks that the short campaign is beatable.
  for (bool hard : {false, true}) {
    int wins = 0;
    for (int seed = 0; seed < 100; ++seed) {
      std::srand(seed); g.start(hard);
      for (int steps = 0; steps < 3000 && !g.over; ++steps) {
        bool choose = true, end = false;
        if (g.phase == RogueCards::COMBAT) {
          int action = -1;
          // Heal before a killing blow if the hand contains a healing card.
          for (int i = 0; i < g.handSize; ++i)
            if (g.hand[i] == RogueCards::HEAL && g.hp < g.maxHp) action = i;
          if (action < 0 && g.handSize) action = 0;
          if (action < 0) {
            int index = g.handSize;
            for (int a = 0; a < 6; ++a) if (g.actives[a]) {
              if (!g.activeUsed[a] &&
                  (a != RogueCards::MEND || g.hp <= g.maxHp - 5) &&
                  (a != RogueCards::WARD || g.block < g.intent())) action = index;
              ++index;
            }
          }
          if (action < 0) { choose = false; end = true; }
          else g.selected = action;
        } else if (g.phase == RogueCards::PASSIVE || g.phase == RogueCards::ACTIVE) {
          const int passiveWeights[] = {10, 7, 5, 3, 8, 6};
          const int activeWeights[] = {10, 7, 8, 12, 9, 8};
          const int *weights = g.phase == RogueCards::PASSIVE ? passiveWeights : activeWeights;
          g.selected = 0;
          for (int i = 1; i < 3; ++i) if (weights[g.choices[i]] > weights[g.choices[g.selected]]) g.selected = i;
        }
        tick(g, 0, 0, choose, end);
      }
      assert(g.over);
      if (g.won) ++wins;
    }
    assert(wins > 0);
    std::cout << (hard ? "Hard" : "Easy") << " campaign policy: " << wins << "/100 completed runs won.\n";
  }
  std::cout << "PASS: Rogue Cards turns, energy, card deck, passives, active upgrades/cooldowns, poison/thorns, all boss rewards, events, win/loss, reset, rollover, 40000 input/draw steps.\n";
}
