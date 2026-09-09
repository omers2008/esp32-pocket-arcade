#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Original tiny arena brawler, not a port of Street Fighter II or its assets.
class StreetFighter {
 public:
  uint32_t score = 0, wave = 1;
  int hp = 8;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; wave = 1; hp = hard ? 6 : 8; over = false;
    startWave();
  }

  void update(int stickX, int stickY, bool punchPressed, bool kickPressed) {
    if (over) return;
    uint32_t now = millis();
    if (stickY < 350) upReady = true;
    bool jump = stickY > 650 && upReady;
    if (jump) upReady = false;
    if (now - waveAt < 800) return; // Ignore action presses during READY.
    jumpQueued |= jump; punchQueued |= punchPressed; kickQueued |= kickPressed;
    if (now - lastFrame < 20) return;
    lastFrame = now;
    if (immune) --immune;
    if (attackTicks) --attackTicks;
    if (cooldown) --cooldown;
    int move = stickX > 650 ? 1 : stickX < -650 ? -1 : 0;
    if (move && !attackTicks) facing = move;
    x = constrain(x + move * (attackTicks ? 0.6f : 1.6f), 2.0f, 118.0f);
    if (jumpQueued && grounded) { vy = -3.8f; grounded = false; }
    jumpQueued = false;
    vy += 0.26f; y += vy;
    if (y >= 46) { y = 46; vy = 0; grounded = true; }
    if (!cooldown && (punchQueued || kickQueued)) {
      kicking = kickQueued;
      attackTicks = kicking ? 8 : 6;
      cooldown = kicking ? 24 : 12;
      for (auto &e : enemies) e.hit = false;
    }
    punchQueued = kickQueued = false;

    // Newly spawned enemies have a short grace period before they can strike.
    if (toSpawn && now - lastSpawn >= (hard ? 900u : 1300u)) {
      for (int i = 0; i < enemyLimit(); ++i) if (!enemies[i].active) {
        spawnEnemy(i); lastSpawn = now; break;
      }
    }
    for (int i = 0; i < enemyLimit(); ++i) {
      Enemy &e = enemies[i]; if (!e.active) continue;
      if (attackTicks && !e.hit && attackHits(e)) {
        e.hit = true; e.hp -= kicking ? 2 : 1;
        e.x = constrain(e.x + facing * (kicking ? 8.0f : 5.0f), 2.0f, 118.0f);
        e.stun = 10; e.windup = e.strike = 0;
        if (e.hp <= 0) { e.active = false; score += e.type == 2 ? 200 : e.type == 1 ? 150 : 100; continue; }
      }
      if (e.stun) { --e.stun; continue; }
      if (e.cooldown) --e.cooldown;
      if (e.strike) { --e.strike; continue; }
      if (e.windup) {
        if (--e.windup == 0) {
          e.strike = 8; e.cooldown = hard ? 35 : 55;
          float strikeX = e.facing > 0 ? e.x + 6 : e.x - 9;
          if (!immune && overlaps(strikeX, 49, 10, 6, x, y, 8, 14)) {
            hp = max(0, hp - (hard && e.type == 2 ? 2 : 1));
            immune = hard ? 25 : 35;
            x = constrain(x + e.facing * 3.0f, 2.0f, 118.0f);
            attackTicks = 0;
            if (!hp) { over = true; return; }
          }
        }
        continue;
      }
      float delta = x - e.x;
      e.facing = delta >= 0 ? 1 : -1;
      if (abs(delta) > 12) {
        float speed = e.type == 2 ? (hard ? 0.45f : 0.3f) :
                      e.type == 1 ? (hard ? 1.0f : 0.75f) : (hard ? 0.65f : 0.45f);
        e.x = constrain(e.x + e.facing * speed, 2.0f, 118.0f);
      } else if (!e.cooldown) {
        e.windup = hard ? 12 : 18;
      }
    }
    if (!toSpawn && liveEnemies() == 0) {
      score += 250; ++wave; hp = min(hp + 1, hard ? 6 : 8); startWave();
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    d.setCursor(0, 0); d.print(score);
    d.setCursor(54, 0); d.print(F("W")); d.print(wave);
    d.setCursor(90, 0); d.print(F("HP")); d.print(hp);
    d.setCursor(121, 0); d.print(hard ? F("H") : F("E"));
    d.drawFastHLine(0, 9, 128, SSD1306_WHITE);
    d.setCursor(0, 12); d.print(F("FOES ")); d.print(toSpawn + liveEnemies());
    // Sparse skyline leaves the fighters readable against a mostly black arena.
    d.drawRect(5, 26, 18, 34, SSD1306_WHITE);
    d.drawRect(105, 23, 17, 37, SSD1306_WHITE);
    for (int wy = 29; wy < 48; wy += 9) {
      d.drawRect(10, wy, 4, 4, SSD1306_WHITE); d.drawRect(110, wy, 4, 4, SSD1306_WHITE);
    }
    d.drawFastHLine(0, 60, 128, SSD1306_WHITE);
    for (int i = 0; i < enemyLimit(); ++i) {
      Enemy &e = enemies[i]; if (!e.active) continue;
      fighter(d, int(e.x), 46, e.facing, false, e.strike > 0, false);
      d.drawFastHLine(int(e.x), 42, e.hp * 2, SSD1306_WHITE);
      if (e.windup) { d.setCursor(int(e.x) + 2, 33); d.print(F("!")); }
    }
    if (!immune || immune % 8 < 4) fighter(d, int(x), int(y), facing, true, attackTicks > 0, kicking);
    if (millis() - waveAt < 800) {
      d.fillRect(39, 26, 51, 14, SSD1306_BLACK);
      d.setCursor(46, 29); d.print(F("FIGHT!"));
    }
    d.display();
  }

 private:
  struct Enemy {
    float x = 0;
    int hp = 0, type = 0, facing = -1, windup = 0, strike = 0, cooldown = 0, stun = 0;
    bool active = false, hit = false;
  };
  Enemy enemies[3];
  float x = 60, y = 46, vy = 0;
  int facing = 1, attackTicks = 0, cooldown = 0, immune = 0, toSpawn = 0, spawned = 0;
  bool hard = false, grounded = true, kicking = false, upReady = false;
  bool jumpQueued = false, punchQueued = false, kickQueued = false;
  uint32_t lastFrame = 0, waveAt = 0, lastSpawn = 0;

  int enemyLimit() const { return hard ? 3 : 2; }
  int liveEnemies() const {
    int count = 0; for (const auto &e : enemies) if (e.active) ++count; return count;
  }
  static bool overlaps(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
  }
  bool attackHits(const Enemy &e) const {
    int reach = kicking ? 14 : 9;
    float ax = facing > 0 ? x + 7 : x - reach;
    return overlaps(ax, y + (kicking ? 9 : 5), reach, 5, e.x, 46, 8, 14);
  }
  void startWave() {
    for (auto &e : enemies) e = Enemy{};
    toSpawn = 3 + int(min(wave, uint32_t(7))); spawned = 0;
    x = 60; y = 46; vy = 0; facing = 1; grounded = true;
    attackTicks = cooldown = immune = 0; kicking = false; upReady = false;
    jumpQueued = punchQueued = kickQueued = false;
    waveAt = lastFrame = lastSpawn = millis();
  }
  void spawnEnemy(int slot) {
    Enemy &e = enemies[slot]; e = Enemy{};
    e.type = wave > 1 ? spawned % 3 : 0;
    bool left = spawned % 2 != 0;
    if (x < 25) left = false;
    if (x > 95) left = true;
    e.x = left ? 2 : 118;
    e.facing = left ? 1 : -1;
    e.hp = (e.type == 2 ? 4 : 2) + (hard ? 1 : 0);
    e.active = true; e.cooldown = 35;
    // Don't let an attack already in progress hit a newly reused enemy slot.
    e.hit = attackTicks > 0;
    ++spawned; --toSpawn;
  }
  static void fighter(Adafruit_SSD1306 &d, int px, int py, int facing, bool player, bool attack, bool kick) {
    d.fillRect(px + 2, py, 4, 4, SSD1306_WHITE);
    d.drawPixel(px + (facing > 0 ? 5 : 2), py + 1, SSD1306_BLACK);
    if (player) d.fillRect(px + 1, py + 4, 6, 6, SSD1306_WHITE);
    else d.drawRect(px + 1, py + 4, 6, 6, SSD1306_WHITE);
    int handX = px + (facing > 0 ? 7 : 0);
    d.drawLine(px + 4, py + 5, attack && !kick ? handX + facing * 8 : handX, py + 6, SSD1306_WHITE);
    d.drawLine(px + 2, py + 9, px, py + 13, SSD1306_WHITE);
    d.drawLine(px + 5, py + 9, attack && kick ? handX + facing * 13 : px + 7,
               attack && kick ? py + 10 : py + 13, SSD1306_WHITE);
  }
};
