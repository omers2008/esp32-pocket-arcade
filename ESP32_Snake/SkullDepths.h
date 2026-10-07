#pragma once
#include <Arduino.h>
#include "ArcadeDisplay.h"
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
    // Rendering can span several ticks. Keep combat at 50 Hz, with a bounded
    // catch-up after stalls; a queued dash is consumed only once.
    if (now - lastFrame > 100) lastFrame = now - 100;
    while (now - lastFrame >= 20 && phase == FIGHT && !over) {
      lastFrame += 20;
      tick(sx, sy, attackQueued || attackHeld, dashQueued);
      attackQueued = dashQueued = false;
    }
  }

  void draw(ArcadeDisplay &d) {
#if defined(ARDUINO) || defined(ARCADE_TEST_TFT)
    if(d.colorFrame([&](Adafruit_GFX &canvas){drawColor(canvas);}))return;
#endif
    d.clearDisplay(); d.setTextColor(SSD1306_WHITE); d.setTextSize(1);
    if (phase == INTRO) {
      text(d, 0, "SKULL DEPTHS"); text(d, 11, "Stick: move");
      text(d, 22, "13 auto-aim sword"); text(d, 33, "14 dash: avoid hits");
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
      for (int i = 0; i < 4; ++i) {
        if(i==selection)d.fillRect(0,11+i*10,128,10,Ink::Purple);
        d.setTextColor(i==selection?Ink::Dark:Ink::White);
        d.setCursor(0, 12 + i * 10); d.print(i == selection ? '>' : ' '); d.print(items[i]);
      }
      text(d, 56, shopNotice ? "Can't buy this" : "Stick / 13 confirm");
    } else {
      d.drawRect(1, 10, 126, 45, Ink::Wall);
      for (int i = 0; i < 2; ++i) { Block b = block(i); d.drawRect(b.x, b.y, b.w, b.h, Ink::Wall); }
      for (const auto &e : enemies) if (e.hp > 0) {
        skull(d, e);
        if (e.windup) {
          if (!e.archer && (!e.boss || e.pattern % 2 == 0))
            d.drawCircle(int(e.tx), int(e.ty), e.boss ? 10 : 7, Ink::Red);
          else d.drawLine(int(e.x), int(e.y), int(e.x + e.ax * 8), int(e.y + e.ay * 8), Ink::Red);
        }
      }
      for (const auto &s : shots) if (s.life) {
        d.drawLine(int(s.x), int(s.y), int(s.x - s.vx * 1.5f), int(s.y - s.vy * 1.5f), s.friendly?Ink::Gold:Ink::Red);
        if (!s.friendly) d.drawPixel(int(s.x + 1), int(s.y), Ink::Red);
      }
      if (!immune || frames % 6 < 3) {
        if (stealth) d.drawRect(int(x) - 2, int(y) - 2, 5, 5, Ink::Purple);
        else d.fillRect(int(x) - 2, int(y) - 2, 5, 5, Ink::Cyan);
        d.drawPixel(int(x + fx * 4), int(y + fy * 4), Ink::Cyan);
      }
      if (dashTicks) d.drawLine(int(x), int(y), int(x - dx * 6), int(y - dy * 6), Ink::Blue);
      if (slash) {
        float r = reach();
        d.drawLine(int(x + slashFx * r - slashFy * 5), int(y + slashFy * r + slashFx * 5),
                   int(x + slashFx * r + slashFy * 5), int(y + slashFy * r - slashFx * 5), Ink::Gold);
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
  struct Enemy { float x, y, tx, ty, ax, ay; int hp, maxHp, cooldown, windup, pattern; bool archer, boss; int stagger=0, flash=0; };
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
  int combo=0,comboTimer=0,parryFlash=0,damageFlash=0;
  bool heavySlash=false;
  float x = 12, y = 32, fx = 1, fy = 0, dx = 1, dy = 0;
  float slashFx = 1, slashFy = 0;
  uint32_t lastFrame = 0, frames = 0;

  const char *areaName() const { return stage == 1 ? "CRYPT" : stage == 2 ? "RUINS" : "KEEP"; }
  int maxCharges() const { return 1 + perks[DASH]; }
  int damage() const { return 2 + perks[MIGHT]; }
  float reach() const { return 13.0f + perks[REACH] * 2; }
  static float dist2(float ax, float ay, float bx, float by) { float u = ax - bx, v = ay - by; return u*u + v*v; }
  static void unit(float &u, float &v) { float n = sqrtf(u*u + v*v); if (n > 0.001f) { u /= n; v /= n; } else { u = 1; v = 0; } }
  Block block(int i) const {
    if (stage == 1) return {48 + i * 32, 24 + i * 23 + (room % 2) * 5, 5, 10};
    if (stage == 2) return {40 + i * 48, 20 + i * 30, 12, 5};
    return {45 + i * 32, 20 + ((i + room) % 2) * 34, 7, 10};
  }
  bool solid(float px, float py, float radius) const {
    if (px < 3 + radius || px > 125 - radius || py < 12 + radius || py > 77 - radius) return true;
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
  bool coverBetween(float ax,float ay,float bx,float by) const {
    int steps=max(1,int(ceilf(sqrtf(dist2(ax,ay,bx,by)))));
    for(int n=1;n<steps;++n)if(solid(ax+(bx-ax)*n/steps,ay+(by-ay)*n/steps,0))return true;
    return false;
  }
  void resetArena() {
    phase = FIGHT; x = 12; y = 32; fx = dx = 1; fy = dy = 0;
    slashFx = 1; slashFy = 0;
    charges = maxCharges(); recharge = dashTicks = stealth = slow = attackCooldown = slash = 0;
    combo=comboTimer=parryFlash=damageFlash=0;heavySlash=false;
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
      e.x = float(70 + (i % 3) * 19); e.y = float(20 + (i / 3) * 22);
      for (int tries = 0; solid(e.x, e.y, e.boss ? 5 : 3) && tries < 80; ++tries) {
        e.x = float(random(63, 119)); e.y = float(random(18, 71));
      }
      if (solid(e.x, e.y, e.boss ? 5 : 3)) { e.x = 116; e.y = 32; }
      e.cooldown = 35 + i * 12;
    }
  }
  void hurt(int amount) {
    if (immune || dashTicks || over) return;
    hp = max(0, hp - amount); immune = 45;
    damageFlash=10;combo=comboTimer=0;
    if (!hp) over = true;
  }
  void hit(Enemy &e, int amount, bool melee) {
    if (e.hp <= 0) return;
    e.hp = max(0, e.hp - amount);
    e.flash=7;
    if (!e.hp) {
      score += e.boss ? 500 : 50;
      gold += (e.boss ? 30 : 6) + perks[GOLD] * 5;
      if (melee && perks[LEECH] && ++meleeKills % 6 == 0) hp = min(maxHp, hp + 1);
    }
  }
  void swing() {
    combo=comboTimer?combo%3+1:1;comboTimer=50;
    heavySlash=combo==3;
    attackCooldown = max(8, 22 - perks[HASTE] * 4)+(heavySlash?6:0); slash = heavySlash?10:7;
    float range=reach()+(heavySlash?3:0);
    slashFx=fx;slashFy=fy;
    Enemy *target=nullptr;float closest=range*range;
    for(auto &e:enemies)if(e.hp>0) {
      float distance=dist2(x,y,e.x,e.y);
      if(distance<=closest && !coverBetween(x,y,e.x,e.y)) {
        target=&e;closest=distance;
      }
    }
    if(target) {
      slashFx=target->x-x;slashFy=target->y-y;
      unit(slashFx,slashFy);
    }
    bool empowered = stealth > 0, connected = false;
    for (auto &e : enemies) if (e.hp > 0) {
      float vx = e.x - x, vy = e.y - y;
      if (vx*vx + vy*vy <= range*range && vx*slashFx + vy*slashFy >= -2 && !coverBetween(x,y,e.x,e.y)) {
        hit(e, (damage()+(heavySlash?2:0)) * (empowered ? 2 : 1), true); connected = true;
        // Only the finisher interrupts a normal skull's committed attack.
        // Bosses resist stagger, so their warnings remain dangerous.
        if(heavySlash && !e.boss) {e.stagger=18;e.windup=0;e.cooldown=max(e.cooldown,25);}
        if(heavySlash) {unit(vx,vy);move(e.x,e.y,vx*5,vy*5,e.boss?5:3);}
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
    if(e.flash)--e.flash;
    if(e.stagger){--e.stagger;return;}
    if (e.cooldown) --e.cooldown;
    if (e.windup) {
      if (--e.windup == 0) {
        if (e.archer) shoot(e.x, e.y, e.ax, e.ay, false, 1);
        else if (e.boss && e.pattern % 2) {
          // Later bosses shoot more spokes, all announced by the windup.
          int spokes = 4 + stage * 2;
          for (int i = 0; i < spokes; ++i) { float a = i * 6.2831853f / spokes; shoot(e.x, e.y, cosf(a), sinf(a), false, 1); }
        } else if (dist2(x, y, e.tx, e.ty) <= (e.boss ? 100.0f : 49.0f) && !coverBetween(e.x,e.y,x,y)) hurt(e.boss ? 2 : 1);
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
        } else if (dist2(s.x, s.y, x, y) < (dashTicks?36:12)) {
          if(dashTicks) {
            // A dash catches a nearby arrow and sends it back at the nearest foe.
            Enemy *target=nullptr;float best=100000;
            for(auto &e:enemies)if(e.hp>0){float ds=dist2(x,y,e.x,e.y);if(ds<best){best=ds;target=&e;}}
            float vx=target?target->x-s.x:-s.vx,vy=target?target->y-s.y:-s.vy;unit(vx,vy);
            s.friendly=true;s.vx=vx*2.2f;s.vy=vy*2.2f;s.damage=damage();s.life=100;parryFlash=20;
            break;
          }
          hurt(s.damage); s.life = 0;
        }
      }
    }
  }
  void tick(int sx, int sy, bool attack, bool dash) {
    ++frames;
    if(comboTimer && !--comboTimer)combo=0;
    if(parryFlash)--parryFlash;
    if(damageFlash)--damageFlash;
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
  static void text(ArcadeDisplay &d, int y, const char *s) {
    d.setTextColor(y==0?Ink::Gold:y>=55?Ink::Cyan:Ink::White);d.setCursor(0,y);d.print(s);
  }
  static void skull(ArcadeDisplay &d, const Enemy &e) {
    int ex = int(e.x), ey = int(e.y), r = e.boss ? 5 : 3;
    d.fillRect(ex-r, ey-r, r*2+1, r*2, Ink::Skin);
    d.fillRect(ex-r+1, ey, r*2-1, r+2, Ink::Skin);
    d.drawPixel(ex-1, ey-1, SSD1306_BLACK); d.drawPixel(ex+1, ey-1, SSD1306_BLACK);
    d.drawPixel(ex, ey+1, SSD1306_BLACK);
    d.drawPixel(ex-1, ey+r, SSD1306_BLACK); d.drawPixel(ex+1, ey+r, SSD1306_BLACK);
    if (e.archer) { d.drawFastHLine(ex-5, ey-4, 11, Ink::Green); d.fillRect(ex-2, ey-6, 5, 2, Ink::Green); }
    if (e.boss) { d.drawFastHLine(ex-5, ey-7, 11, Ink::Red); d.drawFastHLine(ex-5, ey+7, max(1, e.hp*11/e.maxHp), Ink::Red); }
    if (e.windup) { d.drawFastVLine(ex+6, ey-4, 3, Ink::Orange); d.drawPixel(ex+6, ey, Ink::Orange); }
  }
#if defined(ARDUINO) || defined(ARCADE_TEST_TFT)
  static int screenX(float x) {return int(x*2.5f);}
  static int screenY(float y) {return 49+int((y-12)*2.5f);}
  static void label(Adafruit_GFX &d,int x,int y,const char *s,uint16_t c,int size=1) {
    ColorMenu::label(d,x,y,s,c,size);
  }
  static void bar(Adafruit_GFX &d,int x,int y,int w,int value,int maximum,uint16_t c) {
    d.fillRect(x,y,w,5,Ink::Wall);
    d.fillRect(x,y,w*max(0,min(value,maximum))/max(1,maximum),5,c);
  }
  void drawColor(Adafruit_GFX &d) {
    d.fillScreen(ColorMenu::BG);d.setTextWrap(false);
    if(phase!=FIGHT) {drawMenuColor(d);return;}
    d.setTextColor(Ink::White);d.setTextSize(1);d.setCursor(10,8);
    d.print("HP ");d.print(hp);d.print('/');d.print(maxHp);
    bar(d,10,21,105,hp,maxHp,damageFlash?Ink::Red:Ink::Green);
    label(d,130,8,areaName(),Ink::Gold);
    d.setCursor(178,8);d.print(stage);d.print(room==4?" BOSS":" ROOM ");if(room!=4)d.print(room);
    d.setTextColor(Ink::Gold);d.setCursor(258,8);d.print(gold);d.print('G');
    if(room==4) {
      for(const auto &e:enemies)if(e.boss)bar(d,137,21,171,e.hp,e.maxHp,Ink::Red);
    } else {
      int alive=0;for(const auto &e:enemies)alive+=e.hp>0;
      d.setCursor(137,21);d.setTextColor(Ink::Muted);d.print("SKULLS ");d.print(alive);
      d.setCursor(246,21);d.print(hard?"HARD":"EASY");
    }
    label(d,10,35,parryFlash?"ARROW REFLECTED!":stealth?"STEALTH: NEXT HIT x2":heavySlash && slash?"HEAVY FINISHER":
      comboTimer?"CHAIN YOUR NEXT SWING":"HOLD 13: THREE-HIT COMBO",parryFlash?Ink::Cyan:Ink::Gold);
    for(int n=0;n<3;++n)d.fillRect(274+n*12,34,8,7,combo>n?Ink::Gold:Ink::Wall);
    // The larger arena uses 65 logical rows instead of the old 41.
    d.drawRect(7,48,307,165,damageFlash?Ink::Red:Ink::Wall);
    Adafruit_GFX &screen=d;
    {
    class Arena : public Adafruit_GFX {
     public:
      explicit Arena(Adafruit_GFX &out):Adafruit_GFX(320,240),out(out){}
      void drawPixel(int16_t x,int16_t y,uint16_t c) override {
        if(x>=8 && x<313 && y>=49 && y<212)out.drawPixel(x,y,c);
      }
      void fillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t c) override {
        int left=max(8,int(x)),top=max(49,int(y)),right=min(313,int(x)+w),bottom=min(212,int(y)+h);
        if(right>left && bottom>top)out.fillRect(left,top,right-left,bottom-top,c);
      }
      void drawFastHLine(int16_t x,int16_t y,int16_t w,uint16_t c) override {fillRect(x,y,w,1,c);}
      void drawFastVLine(int16_t x,int16_t y,int16_t h,uint16_t c) override {fillRect(x,y,1,h,c);}
     private:Adafruit_GFX &out;
    } d(screen);
    uint16_t floor=stage==1?0x1085:stage==2?0x1126:0x2084;
    d.fillRect(8,49,305,163,floor);
    for(int yy=53;yy<212;yy+=20)for(int xx=12;xx<312;xx+=20)d.drawPixel(xx,yy,Ink::Wall);
    for(int i=0;i<2;++i){Block b=block(i);int bx=screenX(b.x),by=screenY(b.y);
      d.fillRect(bx,by,int(b.w*2.5f),int(b.h*2.5f),Ink::Wall);
      d.drawFastHLine(bx,by,int(b.w*2.5f),Ink::Muted);
    }
    // Warnings are drawn under actors and show exactly where the attack resolves.
    for(const auto &e:enemies)if(e.hp>0 && e.windup) {
      int ex=screenX(e.x),ey=screenY(e.y);
      if(e.archer){
        float px=e.x,py=e.y;
        for(int n=0;n<100;++n){px+=e.ax;py+=e.ay;if(solid(px,py,0))break;
          if(n%3==0)d.fillRect(screenX(px),screenY(py),2,2,Ink::Red);}
      } else if(e.boss && e.pattern%2) {
        int spokes=4+stage*2;
        for(int n=0;n<spokes;++n){float a=n*6.2831853f/spokes;
          d.drawLine(ex+int(cosf(a)*17),ey+int(sinf(a)*17),ex+int(cosf(a)*29),ey+int(sinf(a)*29),Ink::Orange);}
      } else {
        int tx=screenX(e.tx),ty=screenY(e.ty),r=e.boss?25:17;
        // Clip telegraphs to the arena by using the same world bounds as attacks.
        for(int n=0;n<48;++n){float a=n*6.2831853f/48;int px=tx+int(cosf(a)*r),py=ty+int(sinf(a)*r);
          if(px>=8 && px<313 && py>=49 && py<212)d.drawPixel(px,py,Ink::Red);}
        d.drawLine(tx-3,ty,tx+3,ty,Ink::Red);d.drawLine(tx,ty-3,tx,ty+3,Ink::Red);
      }
      bar(d,ex-10,ey-20,20,e.windup,e.boss?35:hard?22:30,Ink::Orange);
    }
    for(const auto &e:enemies)if(e.hp>0) {
      int ex=screenX(e.x),ey=screenY(e.y),r=e.boss?10:6;
      uint16_t color=e.flash?Ink::White:e.boss?Ink::Pink:Ink::Skin;
      d.fillRoundRect(ex-r,ey-r,r*2+1,r*2,3,color);
      d.fillRect(ex-r+2,ey+3,r*2-3,r-1,color);
      d.fillRect(ex-4,ey-3,3,4,Ink::Dark);d.fillRect(ex+2,ey-3,3,4,Ink::Dark);
      d.drawFastVLine(ex-2,ey+r-3,3,Ink::Dark);d.drawFastVLine(ex+2,ey+r-3,3,Ink::Dark);
      if(e.archer){d.fillRect(ex-8,ey-8,17,3,Ink::Green);d.fillRect(ex-4,ey-12,9,5,Ink::Green);}
      if(e.boss){d.fillRect(ex-10,ey-14,21,3,Ink::Gold);for(int i=-1;i<2;++i)d.fillRect(ex+i*8-1,ey-18,3,5,Ink::Gold);}
      if(e.stagger)label(d,ex-8,ey-21,"***",Ink::Gold);
      if(!e.boss && e.hp<e.maxHp)bar(d,ex-8,ey+10,17,e.hp,e.maxHp,Ink::Red);
    }
    for(const auto &s:shots)if(s.life){
      int sx=screenX(s.x),sy=screenY(s.y);
      d.drawLine(sx,sy,screenX(s.x-s.vx*2),screenY(s.y-s.vy*2),s.friendly?Ink::Cyan:Ink::Red);
      d.fillRect(sx-1,sy-1,3,3,s.friendly?Ink::White:Ink::Orange);
    }
    int px=screenX(x),py=screenY(y);
    if(dashTicks){for(int n=1;n<=3;++n)d.drawCircle(screenX(x-dx*n*2),screenY(y-dy*n*2),5,Ink::Blue);}
    if(!immune || frames%6<3) {
      d.fillRoundRect(px-5,py-5,11,12,2,stealth?Ink::Purple:Ink::Cyan);
      d.fillRect(px-3,py-7,7,5,Ink::Skin);
      d.fillRect(px-3,py+6,3,3,Ink::Blue);d.fillRect(px+2,py+6,3,3,Ink::Blue);
      float swordX=slash?slashFx:fx,swordY=slash?slashFy:fy;
      d.drawLine(px+int(swordX*6),py+int(swordY*6),px+int(swordX*11),py+int(swordY*11),Ink::White);
    }
    if(slash){float angle=atan2f(slashFy,slashFx),range=(reach()+(heavySlash?3:0))*2.5f;
      for(int n=-6;n<6;++n){float a=angle+n*0.16f,b=angle+(n+1)*0.16f;
        int ax=px+int(cosf(a)*range),ay=py+int(sinf(a)*range),bx=px+int(cosf(b)*range),by=py+int(sinf(b)*range);
        if(ax>=8 && ax<313 && bx>=8 && bx<313 && ay>=49 && ay<212 && by>=49 && by<212)
          d.drawLine(ax,ay,bx,by,heavySlash?Ink::Gold:Ink::White);}
    }
    }
    // Repaint outer HUD bands over effects that reach the arena edge.
    d.fillRect(0,214,320,26,ColorMenu::BG);
    label(d,10,220,"DASH",Ink::Cyan);
    for(int n=0;n<maxCharges();++n)d.fillRect(42+n*12,219,9,8,n<charges?Ink::Cyan:Ink::Wall);
    if(charges<maxCharges())bar(d,42,231,33,recharge,100,Ink::Cyan);
    label(d,94,220,"13 SWORD / 14 DASH",Ink::White);label(d,94,232,"12 MENU",Ink::Muted);
  }
  void drawMenuColor(Adafruit_GFX &d) {
    label(d,12,12,"SKULL DEPTHS",Ink::Cyan,2);
    if(phase==INTRO) {
      label(d,12,40,"SWORD. DASH. SURVIVE.",Ink::Gold);
      const char *lines[]={"13  Auto-aim sword / hold for combo", "Third swing staggers normal skulls", "14  Dash through danger", "Dash into arrows to reflect them", "Red warnings show enemy attacks", "Clear rooms, pick perks, shop, boss"};
      for(int i=0;i<6;++i)label(d,14,66+i*21,lines[i],i%2?Ink::Muted:Ink::White);
      label(d,14,209,"13  ENTER DUNGEON",Ink::Green,2);return;
    }
    if(phase==REWARD){
      label(d,12,39,"ROOM CLEAR - CHOOSE ONE UPGRADE",Ink::Gold);
      for(int i=0;i<3;++i){int yy=58+i*43,p=choices[i];
        d.fillRoundRect(10,yy,300,37,5,selection==i?ColorMenu::ACTIVE:ColorMenu::CARD);
        if(selection==i)d.drawRoundRect(10,yy,300,37,5,Ink::Cyan);
        label(d,20,yy+6,perkName(p),selection==i?Ink::Cyan:Ink::White,2);
        d.setTextSize(1);d.setTextColor(Ink::Muted);d.setCursor(21,yy+25);d.print("RANK ");d.print(perks[p]);d.print(" -> ");d.print(perks[p]+1);
      }
      label(d,14,190,perkLine(choices[selection],false),Ink::Gold);
      label(d,14,204,perkLine(choices[selection],true),Ink::White);
      label(d,14,227,"STICK CHOOSE   13 TAKE   12 MENU",Ink::Muted);return;
    }
    d.setTextSize(1);d.setTextColor(Ink::Gold);d.setCursor(12,39);d.print("SHOP  GOLD ");d.print(gold);d.print("   HP ");d.print(hp);d.print('/');d.print(maxHp);
    const char *names[]={"Heal 4 HP", "Max health +2", "Sword damage +1", "Enter boss arena"};
    const int prices[]={20,35,40,0};
    for(int i=0;i<4;++i){int yy=58+i*34;
      bool sold=(i==0 && hp==maxHp)||(i==1 && maxHp>=20)||(i==2 && perks[MIGHT]>=cap(MIGHT));
      bool affordable=gold>=prices[i] && !sold;
      d.fillRoundRect(10,yy,300,29,5,i==selection?ColorMenu::ACTIVE:ColorMenu::CARD);
      if(i==selection)d.drawRoundRect(10,yy,300,29,5,Ink::Cyan);
      label(d,20,yy+10,names[i],affordable?Ink::White:Ink::Muted);
      d.setCursor(251,yy+10);d.setTextColor(Ink::Gold);
      if(sold)d.print("FULL");else if(prices[i]){d.print(prices[i]);d.print('G');}else d.print(">>");
    }
    label(d,14,202,shopNotice?"Unavailable or not enough gold":"Prepare for the stage boss",shopNotice?Ink::Red:Ink::Gold);
    label(d,14,226,"STICK CHOOSE   13 BUY/GO   12 MENU",Ink::Muted);
  }
#endif
};
