#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Original miniature whip-and-jump adventure inspired by classic castle games.
class CastleGame {
 public:
  uint32_t score = 0;
  bool over = false, won = false;
  int stage = 1, hp = 6;

  void start(bool hardMode) {
    hard = hardMode;
    score = 0; stage = 1; hp = hard ? 4 : 6;
    over = won = false;
    loadStage();
  }

  void update(int stickX, bool attackHeld, bool jumpPressed) {
    if (over) return;
    jumpQueued |= jumpPressed; // Keep short presses between physics frames.
    uint32_t now = millis();
    if (now - lastFrame < 20) return;
    lastFrame = now;
    if (immune > 0) --immune;
    if (attackTicks > 0) --attackTicks;
    if (cooldown > 0) --cooldown;
    int move = stickX > 650 ? 1 : stickX < -650 ? -1 : 0;
    if (move && attackTicks == 0) facing = move;
    if (jumpQueued && grounded) { vy = -3.5f; grounded = false; }
    jumpQueued = false;
    if (attackHeld && cooldown == 0) {
      attackTicks = 9; cooldown = 20;
      for (auto &e : enemies) e.hit = false;
      bossHit = false;
    }
    x = max(0.0f, min(x + move * (attackTicks ? 0.65f : 1.35f), 632.0f));
    float oldFeet = y + 11;
    vy = min(vy + 0.23f, 4.0f);
    y += vy;
    grounded = false;
    if (vy >= 0) {
      float landing = 100;
      if (floorAt(x + 3) && oldFeet <= 60 && y + 11 >= 60) landing = 60;
      for (auto &p : platforms) {
        if (x + 7 > p.x && x < p.x + p.w && oldFeet <= p.y && y + 11 >= p.y)
          landing = min(landing, float(p.y));
      }
      if (landing < 100) { y = landing - 11; vy = 0; grounded = true; }
    }
    if (grounded && y == 49 && floorAt(x - 12) && floorAt(x + 19)) checkpoint = x;
    if (y > 70) {
      hp -= 2;
      if (hp <= 0) { hp = 0; over = true; return; }
      x = checkpoint; y = 49; vy = 0; immune = 75; grounded = true;
    }
    float whipX = facing > 0 ? x + 7 : x - 20;
    for (auto &c : candles) {
      if (c.alive && attackTicks && overlaps(whipX, y + 3, 20, 5, c.x, c.y, 4, 8)) {
        c.alive = false; score += 50;
        hp = min(hp + 1, hard ? 4 : 6);
      }
    }
    for (auto &e : enemies) {
      if (!e.alive) continue;
      if (abs(e.x - x) < 150) {
        float speed = e.bat ? (hard ? 1.0f : 0.65f) : (hard ? 0.6f : 0.35f);
        float proposed = e.x + e.dir * speed;
        if (proposed < e.origin - 22 || proposed > e.origin + 22 ||
            (!e.bat && !floorAt(proposed + 3))) e.dir = -e.dir;
        else e.x = proposed;
        if (e.bat) {
          e.phase = (e.phase + 1) % 80;
          e.y = 30 + (e.phase < 40 ? e.phase : 80 - e.phase) * 0.45f;
        }
      }
      if (attackTicks && !e.hit && overlaps(whipX, y + 3, 20, 5, e.x, e.y, 7, 9)) {
        e.hit = true;
        if (--e.hp == 0) { e.alive = false; score += e.bat ? 150 : 100; }
      }
      if (e.alive && overlaps(x, y, 7, 11, e.x, e.y, 7, 9)) damage();
    }
    if (bossHp > 0 && x > 470) {
      bossX += bossDir * (hard ? 0.7f : 0.4f);
      if (bossX < 545 || bossX > 606) bossDir = -bossDir;
      if (++bossTimer >= (hard ? 65 : 105)) {
        bossTimer = 0;
        for (auto &b : bolts) if (!b.active) {
          b.x = bossX; b.y = 51; b.vx = x < bossX ? -1.6f : 1.6f; b.active = true; break;
        }
      }
      if (attackTicks && !bossHit && overlaps(whipX, y + 3, 20, 5, bossX, 44, 11, 16)) {
        bossHit = true;
        if (--bossHp == 0) { score += 500; for (auto &b : bolts) b.active = false; }
      }
      if (bossHp > 0 && overlaps(x, y, 7, 11, bossX, 44, 11, 16)) damage();
    }
    for (auto &b : bolts) {
      if (!b.active) continue;
      b.x += b.vx;
      if (abs(b.x - bossX) > 160) { b.active = false; continue; }
      if (attackTicks && overlaps(whipX, y + 3, 20, 5, b.x, b.y, 3, 3)) b.active = false;
      else if (overlaps(x, y, 7, 11, b.x, b.y, 3, 3)) { b.active = false; damage(); }
    }
    if (over) return;
    if (x >= 620 && bossHp == 0) {
      score += 300;
      if (stage == 3) { won = over = true; return; }
      ++stage; hp = min(hp + 1, hard ? 4 : 6); loadStage();
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    int camera = max(0, min(int(x) - 42, 512));
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    // Sparse towers and windows behind the action, below the HUD.
    for (int tower = 0; tower < 640; tower += 64) {
      int sx = tower - camera;
      if (sx < -24 || sx > 127) continue;
      d.drawRect(sx, 19, 22, 41, SSD1306_WHITE);
      d.drawRect(sx + 7, 25, 7, 10, SSD1306_WHITE);
      for (int b = 0; b < 3; ++b) d.fillRect(sx + b * 9, 15, 4, 5, SSD1306_WHITE);
    }
    for (int sx = 0; sx < 128; ++sx) if (floorAt(camera + sx))
      d.fillRect(sx, 60, 1, 4, SSD1306_WHITE);
    for (auto &p : platforms) d.fillRect(p.x - camera, p.y, p.w, 2, SSD1306_WHITE);
    for (auto &c : candles) if (c.alive) {
      int sx = c.x - camera;
      d.fillRect(sx, c.y + 3, 3, 5, SSD1306_WHITE);
      d.drawPixel(sx + 1, c.y, SSD1306_WHITE);
      d.drawPixel(sx, c.y + 1, SSD1306_WHITE);
    }
    for (auto &e : enemies) if (e.alive) {
      int sx = int(e.x) - camera, sy = int(e.y);
      d.fillRect(sx + 2, sy + 1, 4, e.bat ? 3 : 7, SSD1306_WHITE);
      if (e.bat) {
        int wing = e.phase % 20 < 10 ? -2 : 3;
        d.drawLine(sx - 2, sy + wing, sx + 3, sy + 3, SSD1306_WHITE);
        d.drawLine(sx + 9, sy + wing, sx + 4, sy + 3, SSD1306_WHITE);
      } else {
        d.drawLine(sx, sy + 4, sx + 7, sy + 4, SSD1306_WHITE);
        d.drawPixel(sx + 2, sy + 8, SSD1306_WHITE); d.drawPixel(sx + 5, sy + 8, SSD1306_WHITE);
      }
    }
    if (bossHp > 0) {
      int sx = int(bossX) - camera;
      d.fillRect(sx + 3, 44, 5, 5, SSD1306_WHITE);
      d.drawLine(sx + 3, 48, sx, 59, SSD1306_WHITE);
      d.drawLine(sx + 7, 48, sx + 11, 59, SSD1306_WHITE);
      d.drawLine(sx, 59, sx + 11, 59, SSD1306_WHITE);
      if (x > 470) { d.setCursor(73, 11); d.print(F("BOSS ")); d.print(bossHp); }
    }
    d.drawRect(627 - camera, 42, 11, 18, SSD1306_WHITE);
    if (bossHp > 0) d.drawLine(627 - camera, 42, 637 - camera, 59, SSD1306_WHITE);
    for (auto &b : bolts) if (b.active) d.fillRect(int(b.x) - camera, int(b.y), 3, 3, SSD1306_WHITE);
    if (!immune || immune % 10 < 5) {
      int sx = int(x) - camera, sy = int(y);
      d.fillRect(sx + 2, sy, 4, 3, SSD1306_WHITE);
      d.fillRect(sx + 1, sy + 3, 5, 5, SSD1306_WHITE);
      int stride = grounded && (millis() / 100) % 2 ? 1 : 0;
      d.drawLine(sx + 2, sy + 7, sx + stride, sy + 10, SSD1306_WHITE);
      d.drawLine(sx + 4, sy + 7, sx + 6 - stride, sy + 10, SSD1306_WHITE);
    }
    if (attackTicks) {
      int wx = int(facing > 0 ? x + 7 : x - 20) - camera;
      d.drawLine(wx, int(y) + 5, wx + 19, int(y) + 5, SSD1306_WHITE);
      d.fillRect(facing > 0 ? wx + 18 : wx, int(y) + 4, 2, 3, SSD1306_WHITE);
    }
    d.fillRect(0, 0, 128, 9, SSD1306_BLACK);
    d.setCursor(0, 0); d.print(F("HP")); d.print(hp);
    d.setCursor(27, 0); d.print(score);
    d.setCursor(82, 0); d.print(F("S")); d.print(stage);
    d.setCursor(115, 0); d.print(hard ? F("H") : F("E"));
    d.display();
  }

 private:
  struct Platform { int x, y, w; };
  struct Enemy { float x, y, origin; int dir, hp, phase; bool bat, alive, hit; };
  struct Candle { int x, y; bool alive; };
  struct Bolt { float x, y, vx; bool active; };
  Platform platforms[5]; Enemy enemies[6]; Candle candles[3]; Bolt bolts[3];
  float x = 10, y = 49, vy = 0, checkpoint = 10, bossX = 580;
  int facing = 1, attackTicks = 0, cooldown = 0, immune = 0;
  int bossHp = 0, bossDir = -1, bossTimer = 0;
  bool hard = false, grounded = true, jumpQueued = false, bossHit = false;
  uint32_t lastFrame = 0;

  bool floorAt(float px) const {
    int shift = (stage - 1) * 6;
    return px >= 0 && px < 640 && !(px >= 160 + shift && px < 184 + shift) &&
           !(px >= 350 - shift && px < 378 - shift);
  }
  static bool overlaps(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
  }
  void damage() {
    if (immune || over) return;
    --hp; immune = hard ? 45 : 65;
    if (hp <= 0) { hp = 0; over = true; }
  }
  void loadStage() {
    x = checkpoint = 10; y = 49; vy = 0; facing = 1;
    grounded = true; jumpQueued = false;
    attackTicks = cooldown = 0; immune = 50;
    bossX = 580; bossDir = -1; bossTimer = 0; bossHit = false;
    bossHp = (hard ? 7 : 4) + stage - 1;
    platforms[0] = {100, 43, 30}; platforms[1] = {205, 43, 32};
    platforms[2] = {270, 31, 32}; platforms[3] = {390, 43, 40};
    platforms[4] = {460, 31, 28};
    const int spawnX[] = {78, 225, 305, 415, 510, 490};
    for (int i = 0; i < 6; ++i) {
      bool bat = i == 2 || i == 4;
      enemies[i] = {float(spawnX[i]), bat ? 35.0f : 51.0f, float(spawnX[i]), -1,
                    hard && !bat ? 2 : 1, i * 10, bat, true, false};
    }
    candles[0] = {115, 35, true}; candles[1] = {285, 23, true}; candles[2] = {465, 23, true};
    for (auto &b : bolts) b.active = false;
    lastFrame = millis();
  }
};
