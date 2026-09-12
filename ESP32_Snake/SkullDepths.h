#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// Three areas: three arenas -> shop -> boss. All combat timers tick at 50 Hz;
// reward/shop screens pause combat and require a fresh confirmation press.
class SkullDepths {
 public:
  uint32_t score = 0;
  bool over = false, won = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; over = won = false;
    stage = room = 1; hp = maxHp = hard ? 6 : 8; gold = 0;
    for (auto &p : perks) p = 0;
    phase = INTRO; selection = 0; meleeKills = 0;
    lastAttack = lastDash = attackQueued = dashQueued = false;
    menuReady = false; lastFrame = millis();
    resetArena(); phase = INTRO;
  }

  void update(int sx, int sy, bool attackHeld, bool dashHeld) {
    if (over) return;
    bool confirm = attackHeld && !lastAttack;
    bool dashPress = dashHeld && !lastDash;
    lastAttack = attackHeld; lastDash = dashHeld;
    if (phase != FIGHT) {
      attackQueued = dashQueued = false; lastFrame = millis();
      if (phase == INTRO) { if (confirm) resetArena(); return; }
      if (abs(sx) < 350 && abs(sy) < 350) menuReady = true;
      if (menuReady && (abs(sx) > 650 || abs(sy) > 650)) {
        int step = abs(sy) > abs(sx) ? (sy > 0 ? -1 : 1) : (sx > 0 ? 1 : -1);
        int count = phase == REWARD ? 3 : 4;
        selection = (selection + count + step) % count; menuReady = false;
      }
      if (confirm) {
        if (phase == REWARD) { grant(choices[selection]); advance(); }
        else buy();
      }
      return;
    }
    attackQueued |= attackHeld; dashQueued |= dashPress;
    uint32_t now = millis();
    if (now - lastFrame < 20) return;
    lastFrame = now;
    tick(sx, sy, attackQueued, dashQueued);
    attackQueued = dashQueued = false;
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextColor(SSD1306_WHITE); d.setTextSize(1);
    if (phase == INTRO) {
      text(d, 0, "SKULL DEPTHS"); text(d, 11, "Stick: move / aim");
      text(d, 22, "13 sword / 14 dash"); text(d, 33, "Dash avoids damage");
      text(d, 44, "Rooms > shop > boss"); text(d, 55, "13: enter dungeon");
    } else if (phase == REWARD) {
      text(d, 0, "ROOM CLEAR: PICK 1");
      d.setCursor(0, 11); d.print(selection + 1); d.print(F("/3 ")); d.print(perkName(choices[selection]));
      text(d, 24, perkLine(choices[selection], false));
      text(d, 35, perkLine(choices[selection], true));
      text(d, 46, "Stick: choose"); text(d, 56, "13: take upgrade");
    } else if (phase == SHOP) {
      d.setCursor(0, 0); d.print(F("SHOP G")); d.print(gold); d.print(F(" H")); d.print(hp); d.print('/'); d.print(maxHp);
      static const char *const items[] = {"Heal 4       20G", "Max HP +2    35G", "Sword +1     40G", "Enter boss arena"};
      for (int i = 0; i < 4; ++i) { d.setCursor(0, 12 + i * 10); d.print(i == selection ? '>' : ' '); d.print(items[i]); }
      text(d, 56, shopNotice ? "Can't buy this" : "Stick / 13 confirm");
    } else {
      d.drawRect(1, 10, 126, 45, SSD1306_WHITE);
      for (int i = 0; i < 2; ++i) { Block b = block(i); d.drawRect(b.x, b.y, b.w, b.h, SSD1306_WHITE); }
      for (const auto &e : enemies) if (e.hp > 0) {
        skull(d, e);
        if (e.windup) {
          if (!e.archer && (!e.boss || e.pattern % 2 == 0))
            d.drawCircle(int(e.tx), int(e.ty), e.boss ? 10 : 7, SSD1306_WHITE);
          else d.drawLine(int(e.x), int(e.y), int(e.x + e.ax * 8), int(e.y + e.ay * 8), SSD1306_WHITE);
        }
      }
      for (const auto &s : shots) if (s.life) {
        d.drawLine(int(s.x), int(s.y), int(s.x - s.vx * 1.5f), int(s.y - s.vy * 1.5f), SSD1306_WHITE);
        if (!s.friendly) d.drawPixel(int(s.x + 1), int(s.y), SSD1306_WHITE);
      }
      if (!immune || frames % 6 < 3) {
        if (stealth) d.drawRect(int(x) - 2, int(y) - 2, 5, 5, SSD1306_WHITE);
        else d.fillRect(int(x) - 2, int(y) - 2, 5, 5, SSD1306_WHITE);
        d.drawPixel(int(x + fx * 4), int(y + fy * 4), SSD1306_WHITE);
      }
      if (dashTicks) d.drawLine(int(x), int(y), int(x - dx * 6), int(y - dy * 6), SSD1306_WHITE);
      if (slash) {
        float r = reach();
        d.drawLine(int(x + fx * r - fy * 5), int(y + fy * r + fx * 5),
                   int(x + fx * r + fy * 5), int(y + fy * r - fx * 5), SSD1306_WHITE);
      }
      // Reserve full bands for HUD so attack effects never overlap text.
      d.fillRect(0, 0, 128, 10, SSD1306_BLACK);
      d.setCursor(0, 0); d.print(F("H")); d.print(hp); d.print('/'); d.print(maxHp);
      d.setCursor(54, 0); d.print(F("A")); d.print(stage); d.print(room == 4 ? F(" BOSS") : F(" R")); if (room != 4) d.print(room);
      d.fillRect(0, 55, 128, 9, SSD1306_BLACK);
      d.setCursor(0, 56); d.print(F("D")); d.print(charges); d.print('/'); d.print(maxCharges());
      d.setCursor(36, 56); d.print(F("G")); d.print(gold);
      d.setCursor(78, 56);
      if (stealth) { d.print(F("HIDE ")); d.print((stealth + 49) / 50); }
      else d.print(areaName());
    }
    d.display();
  }

 private:
  enum Phase { INTRO, FIGHT, REWARD, SHOP };
  enum Perk { ARROW, DASH, SHADOW, MIGHT, REACH, HASTE, HEART, MEND, FROST, GOLD, LEECH, PERK_COUNT };
  struct Enemy { float x, y, tx, ty, ax, ay; int hp, maxHp, cooldown, windup, pattern; bool archer, boss; };
  struct Shot { float x, y, vx, vy; int life, damage; bool friendly; };
  struct Block { int x, y, w, h; };
  Enemy enemies[7] = {};
  Shot shots[24] = {};
  int perks[PERK_COUNT] = {}, choices[3] = {};
  Phase phase = INTRO;
  bool hard = false, menuReady = false, shopNotice = false;
  bool lastAttack = false, lastDash = false, attackQueued = false, dashQueued = false;
  int stage = 1, room = 1, hp = 8, maxHp = 8, gold = 0, selection = 0;
  int charges = 1, recharge = 0, dashTicks = 0, immune = 0, stealth = 0;
  int attackCooldown = 0, slash = 0, autoTimer = 50, slow = 0, meleeKills = 0;
  float x = 12, y = 32, fx = 1, fy = 0, dx = 1, dy = 0;
  uint32_t lastFrame = 0, frames = 0;

  const char *areaName() const { return stage == 1 ? "CRYPT" : stage == 2 ? "RUINS" : "KEEP"; }
  int maxCharges() const { return 1 + perks[DASH]; }
  int damage() const { return 2 + perks[MIGHT]; }
  float reach() const { return 13.0f + perks[REACH] * 2; }
  static float dist2(float ax, float ay, float bx, float by) { float u = ax - bx, v = ay - by; return u*u + v*v; }
  static void unit(float &u, float &v) { float n = sqrtf(u*u + v*v); if (n > 0.001f) { u /= n; v /= n; } else { u = 1; v = 0; } }
  Block block(int i) const {
    if (stage == 1) return {48 + i * 32, 22 + (room % 2) * 5, 5, 10};
    if (stage == 2) return {40 + i * 48, 19 + i * 20, 12, 5};
    return {45 + i * 32, 16 + ((i + room) % 2) * 23, 7, 10};
  }
  bool solid(float px, float py, float radius) const {
    if (px < 3 + radius || px > 125 - radius || py < 12 + radius || py > 53 - radius) return true;
    for (int i = 0; i < 2; ++i) {
      Block b = block(i);
      if (px + radius >= b.x && px - radius <= b.x + b.w - 1 && py + radius >= b.y && py - radius <= b.y + b.h - 1) return true;
    }
    return false;
  }
  void move(float &px, float &py, float vx, float vy, float radius) {
    // Substeps keep dashes and fast enemies from crossing cover.
    for (int i = 0; i < 4; ++i) {
      if (!solid(px + vx / 4, py, radius)) px += vx / 4;
      if (!solid(px, py + vy / 4, radius)) py += vy / 4;
    }
  }
  void resetArena() {
    phase = FIGHT; x = 12; y = 32; fx = dx = 1; fy = dy = 0;
    charges = maxCharges(); recharge = dashTicks = stealth = slow = attackCooldown = slash = 0;
    immune = 40; autoTimer = 50; frames = 0; lastFrame = millis();
    attackQueued = dashQueued = false;
    for (auto &s : shots) s = {};
    for (auto &e : enemies) e = {};
    int count = room == 4 ? 1 : min(7, 2 + stage + (room > 1 ? 1 : 0) + int(hard));
    for (int i = 0; i < count; ++i) {
      Enemy &e = enemies[i];
      e.boss = room == 4; e.archer = !e.boss && (i % 3 == 1 || (stage == 3 && i == 3));
      e.hp = e.maxHp = e.boss ? 20 + stage * 8 + (hard ? 10 : 0) : 3 + stage + int(hard);
      // Fixed separated spawn slots on the far side, falling back around cover.
      e.x = float(70 + (i % 3) * 19); e.y = float(18 + (i / 3) * 15);
      for (int tries = 0; solid(e.x, e.y, e.boss ? 5 : 3) && tries < 80; ++tries) {
        e.x = float(random(63, 119)); e.y = float(random(17, 48));
      }
      if (solid(e.x, e.y, e.boss ? 5 : 3)) { e.x = 116; e.y = 32; }
      e.cooldown = 35 + i * 12;
    }
  }
  void hurt(int amount) {
    if (immune || dashTicks || over) return;
    hp = max(0, hp - amount); immune = 45;
    if (!hp) over = true;
  }
  void hit(Enemy &e, int amount, bool melee) {
    if (e.hp <= 0) return;
    e.hp = max(0, e.hp - amount);
    if (!e.hp) {
      score += e.boss ? 500 : 50;
      gold += (e.boss ? 30 : 6) + perks[GOLD] * 5;
      if (melee && perks[LEECH] && ++meleeKills % 6 == 0) hp = min(maxHp, hp + 1);
    }
  }
  void swing() {
    attackCooldown = max(8, 22 - perks[HASTE] * 4); slash = 7;
    bool empowered = stealth > 0, connected = false;
    for (auto &e : enemies) if (e.hp > 0) {
      float vx = e.x - x, vy = e.y - y;
      if (vx*vx + vy*vy <= reach()*reach() && vx*fx + vy*fy >= -2) {
        hit(e, damage() * (empowered ? 2 : 1), true); connected = true;
      }
    }
    if (connected) stealth = 0; // Whiffs preserve the first-hit bonus.
  }
  bool shoot(float px, float py, float vx, float vy, bool friendly, int amount) {
    unit(vx, vy);
    for (auto &s : shots) if (!s.life) {
      float speed = friendly ? 2.2f : (hard ? 1.25f : 1.0f);
      s = {px, py, vx * speed, vy * speed, 130, amount, friendly}; return true;
    }
    return false;
  }
  void autoArrow() {
    Enemy *nearest = nullptr; float best = 100000;
    for (auto &e : enemies) if (e.hp > 0) {
      float ds = dist2(x, y, e.x, e.y);
      if (ds < best) { best = ds; nearest = &e; }
    }
    if (nearest) shoot(x, y, nearest->x - x, nearest->y - y, true, perks[ARROW]);
  }
  void enemyTick(Enemy &e) {
    if (!e.hp) return;
    if (e.cooldown) --e.cooldown;
    if (e.windup) {
      if (--e.windup == 0) {
        if (e.archer) shoot(e.x, e.y, e.ax, e.ay, false, 1);
        else if (e.boss && e.pattern % 2) {
          // Later bosses shoot more spokes, all announced by the windup.
          int spokes = 4 + stage * 2;
          for (int i = 0; i < spokes; ++i) { float a = i * 6.2831853f / spokes; shoot(e.x, e.y, cosf(a), sinf(a), false, 1); }
        } else if (dist2(x, y, e.tx, e.ty) <= (e.boss ? 100.0f : 49.0f)) hurt(e.boss ? 2 : 1);
        ++e.pattern; e.cooldown = e.boss ? 40 : (hard ? 45 : 65);
      }
      return;
    }
    if (stealth) return; // They lose sight; already committed attacks still resolve.
    float vx = x - e.x, vy = y - e.y, ds = vx*vx + vy*vy;
    if (!e.cooldown && (e.archer || (e.boss && e.pattern % 2) || ds < (e.boss ? 280 : 150))) {
      e.tx = x; e.ty = y; e.ax = vx; e.ay = vy; unit(e.ax, e.ay);
      e.windup = e.boss ? 35 : (hard ? 22 : 30); return;
    }
    unit(vx, vy);
    float pace = (hard ? 0.38f : 0.29f) + stage * 0.025f;
    if (slow) pace *= 0.45f;
    if (e.archer) { if (ds < 625) pace = -pace; else if (ds < 2000) pace = 0; }
    else if (ds < 60) pace = 0;
    move(e.x, e.y, vx * pace, vy * pace, e.boss ? 5 : 3);
    // Deliberately no touch damage. Only a resolved attack or arrow hurts.
  }
  void shotTick() {
    for (auto &s : shots) if (s.life) {
      --s.life;
      for (int i = 0; i < 3 && s.life; ++i) {
        s.x += s.vx / 3; s.y += s.vy / 3;
        if (solid(s.x, s.y, 0)) { s.life = 0; break; }
        if (s.friendly) {
          for (auto &e : enemies) if (e.hp && dist2(s.x, s.y, e.x, e.y) < (e.boss ? 36 : 16)) {
            hit(e, s.damage, false); s.life = 0; break;
          }
        } else if (dist2(s.x, s.y, x, y) < 12) { hurt(s.damage); s.life = 0; }
      }
    }
  }
  void tick(int sx, int sy, bool attack, bool dash) {
    ++frames;
    if (immune) --immune;
    if (stealth) --stealth;
    if (slow) --slow;
    if (slash) --slash;
    if (attackCooldown) --attackCooldown;
    if (charges < maxCharges() && ++recharge >= 100) { ++charges; recharge = 0; }
    float vx = abs(sx) > 650 ? float(sx) : 0, vy = abs(sy) > 650 ? -float(sy) : 0;
    bool moving = vx != 0 || vy != 0;
    if (moving) { unit(vx, vy); if (!dashTicks) { fx = vx; fy = vy; } }
    if (dash && charges && !dashTicks) {
      --charges; dashTicks = 10; dx = fx; dy = fy;
      if (perks[SHADOW]) stealth = 250;
      if (perks[FROST]) slow = 100;
    }
    if (dashTicks) move(x, y, dx * 2.6f, dy * 2.6f, 2);
    else if (moving) move(x, y, vx * 0.85f, vy * 0.85f, 2);
    if (attack && !attackCooldown && !dashTicks) swing();
    if (--autoTimer <= 0) { if (perks[ARROW]) autoArrow(); autoTimer = 50; }
    for (auto &e : enemies) enemyTick(e);
    shotTick();
    if (dashTicks) --dashTicks;
    if (over) return;
    bool alive = false; for (const auto &e : enemies) alive |= e.hp > 0;
    if (!alive) cleared();
  }
  void cleared() {
    for (auto &s : shots) s = {};
    score += 100; gold += 10; hp = min(maxHp, hp + perks[MEND]);
    if (room == 4 && stage == 3) { won = over = true; score += 1000; return; }
    phase = REWARD; selection = 0; menuReady = false;
    // Sample without replacement from upgrades that still have useful ranks.
    int pool[PERK_COUNT], n = 0;
    for (int i = 0; i < PERK_COUNT; ++i) if (perks[i] < cap(i)) pool[n++] = i;
    for (int i = 0; i < 3; ++i) { int j = int(random(n)); choices[i] = pool[j]; pool[j] = pool[--n]; }
  }
  static int cap(int p) { return p == SHADOW || p == FROST || p == LEECH ? 1 : p == DASH || p == REACH ? 2 : 3; }
  void grant(int p) {
    if (perks[p] >= cap(p)) return;
    ++perks[p];
    if (p == HEART) { maxHp += 2; hp = min(maxHp, hp + 2); }
  }
  void advance() {
    if (room == 3) { phase = SHOP; selection = 0; menuReady = false; shopNotice = false; }
    else { if (room == 4) { ++stage; room = 1; } else ++room; resetArena(); }
  }
  void buy() {
    shopNotice = false;
    if (selection == 3) { room = 4; resetArena(); return; }
    int price = selection == 0 ? 20 : selection == 1 ? 35 : 40;
    if (gold < price || (selection == 0 && hp == maxHp) || (selection == 1 && maxHp >= 20) || (selection == 2 && perks[MIGHT] >= cap(MIGHT))) { shopNotice = true; return; }
    gold -= price;
    if (selection == 0) hp = min(maxHp, hp + 4);
    else if (selection == 1) { maxHp += 2; hp += 2; }
    else grant(MIGHT);
  }
  static const char *perkName(int p) {
    static const char *const names[] = {"Sentry arrow", "Extra dash", "Shadow dash", "Sharp steel", "Long blade", "Quick hands", "Vital heart", "Room mend", "Frost dash", "Gold hunter", "Soul drinker"};
    return names[p];
  }
  static const char *perkLine(int p, bool second) {
    static const char *const first[] = {"Nearest foe: 1 arrow", "+1 dash charge", "Dash: hide for 5 sec", "+1 sword damage", "+2 sword reach", "Swing faster", "+2 maximum health", "Heal +1 each clear", "Dash slows all foes", "+5 gold per kill", "6 sword kills:"};
    static const char *const next[] = {"each sec; +1 damage", "Each refills in 2s", "First sword hit: x2", "Stacks up to 3", "Stacks twice", "Stacks up to 3", "Also heal 2", "Stacks up to 3", "for 2 seconds", "Stacks up to 3", "heal 1 health"};
    return second ? next[p] : first[p];
  }
  static void text(Adafruit_SSD1306 &d, int y, const char *s) { d.setCursor(0, y); d.print(s); }
  static void skull(Adafruit_SSD1306 &d, const Enemy &e) {
    int ex = int(e.x), ey = int(e.y), r = e.boss ? 5 : 3;
    d.fillRect(ex-r, ey-r, r*2+1, r*2, SSD1306_WHITE);
    d.fillRect(ex-r+1, ey, r*2-1, r+2, SSD1306_WHITE);
    d.drawPixel(ex-1, ey-1, SSD1306_BLACK); d.drawPixel(ex+1, ey-1, SSD1306_BLACK);
    d.drawPixel(ex, ey+1, SSD1306_BLACK);
    d.drawPixel(ex-1, ey+r, SSD1306_BLACK); d.drawPixel(ex+1, ey+r, SSD1306_BLACK);
    if (e.archer) { d.drawFastHLine(ex-5, ey-4, 11, SSD1306_WHITE); d.fillRect(ex-2, ey-6, 5, 2, SSD1306_WHITE); }
    if (e.boss) { d.drawFastHLine(ex-5, ey-7, 11, SSD1306_WHITE); d.drawFastHLine(ex-5, ey+7, max(1, e.hp*11/e.maxHp), SSD1306_WHITE); }
    if (e.windup) { d.drawFastVLine(ex+6, ey-4, 3, SSD1306_WHITE); d.drawPixel(ex+6, ey, SSD1306_WHITE); }
  }
};
