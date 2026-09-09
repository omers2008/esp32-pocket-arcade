#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Original six-room temple adventure; no original Montezuma game code or assets.
class TempleQuest {
 public:
  uint32_t score = 0;
  int room = 0, lives = 5, gems = 0, knives = 6;
  bool over = false, won = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; room = 0; lives = hard ? 3 : 5;
    gems = 0; knives = 6; keys = unlocked = takenKeys = 0;
    over = won = started = false;
    for (auto &g : gemTaken) g = false;
    for (int r = 0; r < 6; ++r) {
      float left = r == 3 || r == 4 ? 90 : r == 5 ? 35 : 15;
      enemies[r][0] = {left + 8, 52, left, left + 22, 1, 0, false, r != 0};
      enemies[r][1] = {80, 19, 55, 112, -1, r * 7, true, r > 1};
    }
    enterRoom(0, 8, 48, false);
  }

  void update(int stickX, int stickY, bool throwPressed, bool jumpPressed) {
    if (over) return;
    uint32_t now = millis();
    if (!started) {
      if (throwPressed) { started = true; lastFrame = now; }
      return;
    }
    throwQueued |= throwPressed; jumpQueued |= jumpPressed;
    if (now - lastFrame < 20) return;
    lastFrame = now;
    if (immune) --immune;
    if (ladderDelay) --ladderDelay;
    int horizontal = stickX > 650 ? 1 : stickX < -650 ? -1 : 0;
    int vertical = stickY > 650 ? -1 : stickY < -650 ? 1 : 0;
    if (horizontal) facing = horizontal;
    int ladder = ladderX(room);
    if (climbing && horizontal) { climbing = false; grounded = false; ladderDelay = 8; }
    if (!climbing && !horizontal && vertical && !ladderDelay && abs(x + 3 - ladder) <= 5 &&
        y <= 49 && y + 10 >= (lowerShaft() ? 9 : 34)) {
      climbing = true; x = ladder - 3; vy = 0; grounded = false;
    }
    if (jumpQueued && (grounded || climbing)) {
      vy = -3.0f; grounded = climbing = false; ladderDelay = 10;
    }
    jumpQueued = false;
    if (climbing) {
      y += vertical * 1.0f;
      if (lowerShaft() && y <= 9 && vertical < 0) {
        throwQueued = false;
        if (tryRoom(room - 3, ladder - 3, 48, false)) return;
      }
      y = constrain(y, lowerShaft() ? 9.0f : 24.0f, 48.0f);
      if ((room == 0 || room == 2) && y >= 48 && vertical > 0) {
        throwQueued = false;
        if (tryRoom(room + 3, ladder - 3, 9, true)) return;
      }
      if (y == 48 || (!lowerShaft() && y == 24)) { climbing = false; grounded = true; }
    } else {
      x += horizontal * 1.4f;
      float feet = y + 10;
      vy = min(vy + 0.2f, 4.0f); y += vy;
      if (y < 10) { y = 10; vy = 0; }
      grounded = false;
      if (vy >= 0) {
        float surface = 100;
        int ledge = platformX(room);
        if (x + 6 > ledge && x < ledge + 44 && feet <= 34 && y + 10 >= 34) surface = 34;
        if (floorAt(room, x + 3) && feet <= 58 && y + 10 >= 58) surface = min(surface, 58.0f);
        if (surface < 100) { y = surface - 10; vy = 0; grounded = true; }
      }
    }
    if (y > 68) { loseLife(); return; }
    // Doorways connect adjacent columns only, at ground level.
    if (y >= 45) {
      if (x < 0 && room % 3 > 0) { if (tryRoom(room - 1, 118, 48, false)) return; }
      if (x > 122 && room % 3 < 2) { if (tryRoom(room + 1, 3, 48, false)) return; }
    }
    x = constrain(x, 0.0f, 122.0f);
    if ((room == 2 || room == 5) && overlaps(x, y, 6, 10, 58, 55, 9, 3)) { loseLife(); return; }

    if (throwQueued && knives > 0) {
      for (auto &d : daggers) if (!d.active) {
        d = {x + 3, y + 5, facing * 3.5f, true}; --knives; break;
      }
    }
    throwQueued = false;
    for (auto &d : daggers) {
      if (!d.active) continue;
      float oldX = d.x; d.x += d.vx;
      if (d.x < -5 || d.x > 132) { d.active = false; continue; }
      for (auto &e : enemies[room]) {
        if (e.alive && overlaps(min(oldX, d.x) - 2, d.y - 1, abs(d.x - oldX) + 5, 3, e.x, e.y, 7, 6)) {
          e.alive = false; d.active = false; score += e.bat ? 75 : 50; break;
        }
      }
    }
    for (auto &e : enemies[room]) {
      if (!e.alive) continue;
      float speed = e.bat ? (hard ? 0.9f : 0.55f) : (hard ? 0.65f : 0.4f);
      e.x += e.dir * speed;
      if (e.x < e.left) { e.x = e.left; e.dir = 1; }
      if (e.x > e.right) { e.x = e.right; e.dir = -1; }
      if (e.bat) { e.phase = (e.phase + 1) % 60; e.y = 17 + (e.phase < 30 ? e.phase : 60 - e.phase) * 0.25f; }
      if (!immune && overlaps(x, y, 6, 10, e.x, e.y, 7, 6)) { loseLife(); return; }
    }
    if (!gemTaken[room] && overlaps(x, y, 6, 10, gemX(room), 26, 5, 6)) {
      gemTaken[room] = true; ++gems; score += 100; knives = min(knives + 1, 9);
    }
    int key = room == 3 ? 0 : room == 1 ? 1 : -1;
    if (key >= 0 && !(takenKeys & (1 << key)) && overlaps(x, y, 6, 10, 106, key == 0 ? 50 : 26, 7, 6)) {
      keys |= 1 << key; takenKeys |= 1 << key; score += 50;
    }
    if (room == 5 && overlaps(x, y, 6, 10, 112, 46, 10, 12)) {
      if (gems == 6) { score += 500 + lives * 100; won = over = true; }
      else { message = 3; messageAt = now; }
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextColor(SSD1306_WHITE); d.setTextSize(1);
    if (!started) {
      line(d, 0, "TEMPLE QUEST"); line(d, 12, "L/R move, U/D climb");
      line(d, 23, "14 jump / 13 dagger"); line(d, 34, "Find keys + 6 gems");
      line(d, 45, "Reach idol in room 6"); line(d, 56, "13: enter temple");
      d.display(); return;
    }
    d.setCursor(0, 0); d.print(score);
    d.setCursor(48, 0); d.print(F("R")); d.print(room + 1);
    d.setCursor(68, 0); d.print(F("L")); d.print(lives);
    d.setCursor(88, 0); d.print(F("G")); d.print(gems);
    d.setCursor(110, 0); d.print(F("D")); d.print(knives);
    d.drawFastHLine(0, 9, 128, SSD1306_WHITE);
    for (int px = 0; px < 128; ++px) if (floorAt(room, px)) d.fillRect(px, 58, 1, 6, SSD1306_WHITE);
    d.fillRect(platformX(room), 34, 44, 2, SSD1306_WHITE);
    int ladder = ladderX(room), top = lowerShaft() ? 10 : 34;
    d.drawFastVLine(ladder - 3, top, 58 - top, SSD1306_WHITE);
    d.drawFastVLine(ladder + 3, top, 58 - top, SSD1306_WHITE);
    for (int ly = top + 3; ly < 58; ly += 5) d.drawFastHLine(ladder - 3, ly, 7, SSD1306_WHITE);
    if (room == 0 || room == 2) {
      d.fillRect(ladder - 3, 58, 7, 6, SSD1306_BLACK);
      d.drawFastVLine(ladder - 3, 58, 6, SSD1306_WHITE); d.drawFastVLine(ladder + 3, 58, 6, SSD1306_WHITE);
    }
    if (room % 3 > 0) d.drawRect(0, 45, 4, 13, SSD1306_WHITE);
    if (room % 3 < 2) d.drawRect(124, 45, 4, 13, SSD1306_WHITE);
    if ((room == 0 || room == 1) && !(unlocked & 1)) {
      d.setCursor(room == 0 ? 118 : 3, 46); d.print('A');
    }
    if ((room == 2 || room == 4 || room == 5) && !(unlocked & 2)) {
      d.setCursor(room == 4 ? 118 : ladder - 2, 46); d.print('B');
    }
    if (room == 2 || room == 5) for (int sx = 58; sx < 67; sx += 3) {
      d.drawLine(sx, 58, sx + 1, 54, SSD1306_WHITE); d.drawLine(sx + 1, 54, sx + 2, 58, SSD1306_WHITE);
    }
    if (!gemTaken[room]) {
      int gx = gemX(room);
      d.drawLine(gx + 2, 26, gx, 29, SSD1306_WHITE); d.drawLine(gx, 29, gx + 2, 32, SSD1306_WHITE);
      d.drawLine(gx + 2, 26, gx + 4, 29, SSD1306_WHITE); d.drawLine(gx + 4, 29, gx + 2, 32, SSD1306_WHITE);
    }
    int key = room == 3 ? 0 : room == 1 ? 1 : -1;
    if (key >= 0 && !(takenKeys & (1 << key))) {
      int ky = key == 0 ? 50 : 26;
      d.drawRect(106, ky, 3, 3, SSD1306_WHITE); d.drawFastHLine(108, ky + 2, 5, SSD1306_WHITE);
      d.drawPixel(112, ky + 3, SSD1306_WHITE);
    }
    for (auto &e : enemies[room]) if (e.alive) {
      int ex = int(e.x), ey = int(e.y);
      d.fillRect(ex + 1, ey + 1, 5, 4, SSD1306_WHITE);
      d.drawPixel(ex + 2, ey + 2, SSD1306_BLACK); d.drawPixel(ex + 4, ey + 2, SSD1306_BLACK);
      if (e.bat) {
        int wing = e.phase % 20 < 10 ? 0 : 5;
        d.drawLine(ex - 2, ey + wing, ex + 3, ey + 2, SSD1306_WHITE);
        d.drawLine(ex + 8, ey + wing, ex + 3, ey + 2, SSD1306_WHITE);
      }
    }
    if (room == 5) {
      d.drawRect(112, 46, 10, 12, SSD1306_WHITE);
      d.drawRect(115, 48, 4, 5, SSD1306_WHITE); d.drawFastHLine(114, 55, 6, SSD1306_WHITE);
    }
    if (!immune || immune % 10 < 5) {
      int px = int(x), py = int(y);
      d.drawFastHLine(px, py, 7, SSD1306_WHITE); d.fillRect(px + 1, py + 1, 4, 3, SSD1306_WHITE);
      d.fillRect(px + 1, py + 4, 4, 3, SSD1306_WHITE);
      d.drawLine(px + 2, py + 6, px, py + 9, SSD1306_WHITE);
      d.drawLine(px + 3, py + 6, px + 5, py + 9, SSD1306_WHITE);
    }
    for (auto &knife : daggers) if (knife.active) d.drawFastHLine(int(knife.x) - 2, int(knife.y), 5, SSD1306_WHITE);
    if (message && millis() - messageAt < 800) {
      d.fillRect(8, 11, 112, 10, SSD1306_BLACK);
      d.setCursor(10, 12); d.print(message == 1 ? "NEED KEY A" : message == 2 ? "NEED KEY B" : "FIND ALL 6 GEMS");
    }
    d.display();
  }

 private:
  struct Enemy { float x, y, left, right; int dir, phase; bool bat, alive; };
  struct Dagger { float x = 0, y = 0, vx = 0; bool active = false; };
  Enemy enemies[6][2]; Dagger daggers[2];
  bool gemTaken[6] = {}, hard = false, started = false, climbing = false, grounded = true;
  bool entryClimbing = false, jumpQueued = false, throwQueued = false;
  int keys = 0, takenKeys = 0, unlocked = 0, facing = 1, immune = 0, ladderDelay = 0, message = 0;
  float x = 8, y = 48, vy = 0, entryX = 8, entryY = 48;
  uint32_t lastFrame = 0, messageAt = 0;

  bool lowerShaft() const { return room == 3 || room == 5; }
  static int ladderX(int r) { return r == 1 || r == 2 || r == 5 ? 96 : 28; }
  static int platformX(int r) { return ladderX(r) == 96 ? 76 : 8; }
  static int gemX(int r) { static const int positions[] = {32, 88, 104, 32, 28, 100}; return positions[r]; }
  static bool floorAt(int r, float px) {
    if (px < 0 || px >= 128) return false;
    if (r == 1 && px >= 60 && px < 78) return false;
    if ((r == 3 || r == 4) && px >= 64 && px < 82) return false;
    return true;
  }
  static bool overlaps(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
  }
  bool tryRoom(int target, float ex, float ey, bool climb) {
    int gate = (room == 0 && target == 1) || (room == 1 && target == 0) ? 1 :
               (room == 5 || target == 5) ? 2 : 0;
    if (gate && !(unlocked & gate)) {
      if (!(keys & gate)) { message = gate; messageAt = millis(); return false; }
      keys &= ~gate; unlocked |= gate;
    }
    enterRoom(target, ex, ey, climb); return true;
  }
  void enterRoom(int target, float ex, float ey, bool climb) {
    room = target; entryX = ex; entryY = ey; entryClimbing = climb;
    respawn(); message = 0; lastFrame = millis();
  }
  void respawn() {
    x = entryX; y = entryY; climbing = entryClimbing; grounded = !climbing;
    vy = 0; immune = hard ? 60 : 90; ladderDelay = 0;
    jumpQueued = throwQueued = false;
    for (auto &d : daggers) d.active = false;
  }
  void loseLife() {
    if (--lives == 0) { over = true; return; }
    respawn();
  }
  static void line(Adafruit_SSD1306 &d, int y, const char *text) { d.setCursor(0, y); d.print(text); }
};
