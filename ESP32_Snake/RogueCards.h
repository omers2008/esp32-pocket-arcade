#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// A short, turn-based run: nine fights, three bosses, and run-long upgrades.
class RogueCards {
 public:
  uint32_t score = 0;
  int battle = 1, hp = 32, maxHp = 32;
  bool over = false, won = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; battle = 1; hp = maxHp = hard ? 26 : 32;
    over = won = false;
    for (auto &p : passives) p = 0;
    for (auto &a : actives) a = 0;
    for (auto &u : activeUsed) u = false;
    const int easyDeck[5] = {STRIKE, STRIKE, SHIELD, SHIELD, HEAL};
    bool hasAttack = false;
    for (int i = 0; i < 5; ++i) {
      starterDeck[i] = hard ? int(random(3)) : easyDeck[i];
      hasAttack |= starterDeck[i] == STRIKE;
    }
    // Avoid a starting hand pool that can never damage the first enemy.
    if (!hasAttack) starterDeck[random(5)] = STRIKE;
    enter(INTRO);
  }

  void update(int stickX, int stickY, bool choosePressed, bool endPressed) {
    if (over) return;
    if (phase == ENEMY) {
      if (millis() - phaseAt >= 600) enemyTurn();
      return;
    }
    if (abs(stickX) < 350 && abs(stickY) < 350) stickReady = true;
    int count = phase == COMBAT ? actionCount() : phase == PASSIVE || phase == ACTIVE ? 3 : 1;
    if (stickReady && (abs(stickX) > 650 || abs(stickY) > 650)) {
      int step = abs(stickY) >= abs(stickX) ? (stickY > 0 ? -1 : 1) : (stickX > 0 ? 1 : -1);
      if (count) selected = (selected + count + step) % count;
      stickReady = false;
    }
    if (millis() - phaseAt < 200) return;
    if (phase == COMBAT && endPressed) { enter(ENEMY); return; }
    if (!choosePressed) return;
    switch (phase) {
      case INTRO: beginBattle(); break;
      case COMBAT: playSelected(); break;
      case PASSIVE:
        gainPassive(choices[selected]);
        if (rewardFromEvent) advance();
        else if (boss()) offer(true, false);
        else afterRewards();
        break;
      case ACTIVE:
        ++actives[choices[selected]];
        if (rewardFromEvent) advance();
        else afterRewards();
        break;
      case EVENT:
        if (eventKind == 0) offer(false, true);
        else if (eventKind == 1) offer(true, true);
        else { heal(7); advance(); }
        break;
      default: break;
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    if (phase == INTRO) {
      line(d, 0, "ROGUE CARDS"); line(d, 13, "Stick: choose card");
      line(d, 24, "13 play / 14 end"); line(d, 35, "5 cards / 3 per turn");
      line(d, 46, "9 fights / 3 bosses"); line(d, 56, "13: begin");
    } else if (phase == COMBAT || phase == ENEMY) {
      d.setCursor(0, 0); d.print(F("HP")); d.print(hp); d.print('/'); d.print(maxHp);
      d.setCursor(54, 0); d.print(F("E")); d.print(energy);
      d.setCursor(78, 0); d.print(F("B")); d.print(block);
      d.setCursor(108, 0); d.print(F("F")); d.print(battle);
      d.setCursor(0, 11); d.print(boss() ? "Boss " : enemyNames()[enemyType]);
      if (!boss()) d.print(' ');
      d.print(enemyHp);
      d.setCursor(78, 11); d.print(F("ATK ")); d.print(intent());
      if (phase == ENEMY) { line(d, 33, "Enemy turn..."); }
      else {
        int count = actionCount(), first = max(0, selected - 2);
        if (!count) line(d, 33, "14: end your turn");
        for (int i = first; i < min(first + 3, count); ++i) {
          int y = 22 + (i - first) * 10;
          highlight(d, y, i == selected);
          d.setCursor(2, y); d.print(i == selected ? '>' : ' ');
          if (i < handSize) {
            d.print(cardName(hand[i])); d.print(F(" 1E"));
          } else {
            int a = activeAt(i - handSize);
            d.print(activeName(a)); d.print(activeUsed[a] ? " USED" : " FREE");
          }
        }
        d.setTextColor(SSD1306_WHITE);
        if (count) {
          d.setCursor(0, 55);
          if (selected < handSize) describeCard(d, hand[selected]);
          else describeActive(d, activeAt(selected - handSize), false);
        }
      }
    } else if (phase == PASSIVE || phase == ACTIVE) {
      line(d, 0, phase == PASSIVE ? "PICK A PASSIVE" : "PICK ACTIVE POWER");
      for (int i = 0; i < 3; ++i) {
        int y = 12 + i * 11;
        highlight(d, y, selected == i);
        d.setCursor(2, y); d.print(i == selected ? '>' : ' ');
        d.print(phase == PASSIVE ? passiveName(choices[i]) : activeName(choices[i]));
        int owned = phase == PASSIVE ? passives[choices[i]] : actives[choices[i]];
        if (owned) { d.print(F(" +")); d.print(owned); }
      }
      d.setTextColor(SSD1306_WHITE); d.setCursor(0, 46);
      if (phase == PASSIVE) d.print(passiveDescription(choices[selected]));
      else describeActive(d, choices[selected], true);
      line(d, 56, "13: take / stick:pick");
    } else if (phase == EVENT) {
      line(d, 0, "RANDOM EVENT");
      line(d, 18, eventKind == 0 ? "A hidden shrine!" : eventKind == 1 ? "A spell cache!" : "A healing spring!");
      line(d, 33, eventKind == 0 ? "Free passive choice" : eventKind == 1 ? "Free active choice" : "Recover 7 HP");
      line(d, 56, "13: claim and go");
    }
    d.display();
  }

 private:
  enum Phase { INTRO, COMBAT, ENEMY, PASSIVE, ACTIVE, EVENT };
  enum Passive { MIGHT, GUARD, HEART, HERBS, THORNS, REST };
  enum Active { FIREBALL, WARD, MEND, VENOM, LEECH, STORM };
  enum Card { STRIKE, SHIELD, HEAL };
  Phase phase = INTRO;
  uint8_t passives[6] = {}, actives[6] = {};
  bool activeUsed[6] = {};
  int starterDeck[5] = {}, hand[3] = {}, handSize = 0, choices[3] = {};
  int selected = 0, enemyHp = 0, enemyType = 0, enemyPoison = 0;
  int energy = 3, block = 0, turn = 0, eventKind = 0;
  bool hard = false, stickReady = false, rewardFromEvent = false;
  uint32_t phaseAt = 0;

  bool boss() const { return battle % 3 == 0; }
  void enter(Phase next) { phase = next; phaseAt = millis(); selected = 0; stickReady = false; }
  void heal(int value) { hp = min(maxHp, hp + value); }
  int intent() const { return min(30, 2 + battle / 2 + (hard ? 1 : 0) + (boss() ? 2 : 0) + turn / 3); }
  int actionCount() const {
    int count = handSize; for (auto a : actives) if (a) ++count; return count;
  }
  int activeAt(int index) const {
    for (int i = 0; i < 6; ++i) if (actives[i] && index-- == 0) return i;
    return -1;
  }
  void beginBattle() {
    enemyType = random(3);
    enemyHp = (boss() ? 30 + battle * 4 : 14 + battle * 3 + random(5)) + (hard ? 4 : 0);
    enemyPoison = 0; turn = 0;
    for (auto &used : activeUsed) used = false;
    newTurn();
  }
  void newTurn() {
    // Draw three from this run's five cards; Hard does not reroll the deck here.
    int deck[5];
    for (int i = 0; i < 5; ++i) deck[i] = starterDeck[i];
    for (int i = 4; i > 0; --i) { int j = random(i + 1); int saved = deck[i]; deck[i] = deck[j]; deck[j] = saved; }
    for (int i = 0; i < 3; ++i) hand[i] = deck[i];
    handSize = 3; energy = 3; block = 0;
    enter(COMBAT);
  }
  void playSelected() {
    if (selected >= actionCount()) return;
    if (selected < handSize) {
      if (!energy) return;
      int card = hand[selected];
      --energy;
      if (card == STRIKE) enemyHp -= 5 + passives[MIGHT];
      else if (card == SHIELD) block += 5 + 2 * passives[GUARD];
      else heal(3 + passives[HERBS]);
      for (int i = selected; i < handSize - 1; ++i) hand[i] = hand[i + 1];
      --handSize;
    } else {
      int a = activeAt(selected - handSize);
      if (a < 0 || activeUsed[a]) return;
      activeUsed[a] = true;
      int boost = 2 * (actives[a] - 1);
      switch (a) {
        case FIREBALL: enemyHp -= 12 + boost + passives[MIGHT]; break;
        case WARD: block += 12 + boost + 2 * passives[GUARD]; break;
        case MEND: heal(8 + boost + passives[HERBS]); break;
        case VENOM: enemyPoison += 4 + boost; break;
        case LEECH: enemyHp -= 8 + boost + passives[MIGHT]; heal(4 + passives[HERBS]); break;
        case STORM: enemyHp -= 6 + boost + passives[MIGHT]; block += 6 + boost; break;
      }
    }
    if (enemyHp <= 0) { victory(); return; }
    selected = min(selected, max(0, actionCount() - 1));
  }
  void enemyTurn() {
    enemyHp -= enemyPoison;
    if (enemyHp <= 0) { victory(); return; }
    hp = max(0, hp - max(0, intent() - block));
    enemyHp -= 2 * passives[THORNS];
    if (!hp) { over = true; return; } // Mutual KO ends the run.
    if (enemyHp <= 0) { victory(); return; }
    ++turn; newTurn();
  }
  void victory() {
    enemyHp = 0;
    score += 100 + battle * 25 + (boss() ? 200 : 0);
    heal((hard ? 0 : 2) + 3 * passives[REST]);
    offer(false, false); // Every battle, including bosses, offers three passives.
  }
  void offer(bool active, bool fromEvent) {
    int pool[6] = {0, 1, 2, 3, 4, 5};
    for (int i = 5; i > 0; --i) { int j = random(i + 1); int saved = pool[i]; pool[i] = pool[j]; pool[j] = saved; }
    for (int i = 0; i < 3; ++i) choices[i] = pool[i];
    rewardFromEvent = fromEvent; enter(active ? ACTIVE : PASSIVE);
  }
  void gainPassive(int p) {
    ++passives[p];
    if (p == HEART) { maxHp += 5; heal(5); }
  }
  void afterRewards() {
    if (battle == 9) { score += 500; won = over = true; return; }
    int roll = random(100);
    if (roll < 55) {
      eventKind = roll < 20 ? 0 : roll < 40 ? 1 : 2;
      enter(EVENT);
    } else advance();
  }
  void advance() { ++battle; beginBattle(); }

  static const char *const *enemyNames() {
    static const char *names[] = {"Slime", "Goblin", "Wisp"}; return names;
  }
  static const char *cardName(int c) { return c == STRIKE ? "Strike" : c == SHIELD ? "Guard" : "Heal"; }
  static const char *passiveName(int p) {
    static const char *names[] = {"Might", "Iron guard", "Big heart", "Herbs", "Thorns", "Campfire"}; return names[p];
  }
  static const char *passiveDescription(int p) {
    static const char *text[] = {"Attacks +1 damage", "Guard/Ward +2 block", "Max HP +5, heal 5", "Healing cards +1 HP", "Reflect 2 per turn", "After fight: heal 3"}; return text[p];
  }
  static const char *activeName(int a) {
    static const char *names[] = {"Fireball", "Ward", "Mend", "Venom", "Leech", "Storm"}; return names[a];
  }
  void describeCard(Adafruit_SSD1306 &d, int c) const {
    if (c == STRIKE) { d.print(5 + passives[MIGHT]); d.print(F(" damage / 1 energy")); }
    else if (c == SHIELD) { d.print(5 + 2 * passives[GUARD]); d.print(F(" block / 1 energy")); }
    else { d.print(3 + passives[HERBS]); d.print(F(" heal / 1 energy")); }
  }
  void describeActive(Adafruit_SSD1306 &d, int a, bool upgrading) const {
    int boost = 2 * max(0, int(actives[a]) - (upgrading ? 0 : 1));
    switch (a) {
      case FIREBALL: d.print(12 + boost + passives[MIGHT]); d.print(F(" damage, once/fight")); break;
      case WARD: d.print(12 + boost + 2 * passives[GUARD]); d.print(F(" block, once/fight")); break;
      case MEND: d.print(8 + boost + passives[HERBS]); d.print(F(" heal, once/fight")); break;
      case VENOM: d.print(4 + boost); d.print(F(" poison every turn")); break;
      case LEECH: d.print(8 + boost + passives[MIGHT]); d.print(F(" dmg + ")); d.print(4 + passives[HERBS]); d.print(F(" heal")); break;
      case STORM: d.print(6 + boost + passives[MIGHT]); d.print(F(" dmg + ")); d.print(6 + boost); d.print(F(" block")); break;
    }
  }
  static void line(Adafruit_SSD1306 &d, int y, const char *text) { d.setCursor(0, y); d.print(text); }
  static void highlight(Adafruit_SSD1306 &d, int y, bool on) {
    if (on) d.fillRect(0, y - 1, 128, 10, SSD1306_WHITE);
    d.setTextColor(on ? SSD1306_BLACK : SSD1306_WHITE);
  }
};
