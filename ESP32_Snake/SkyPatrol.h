#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// Side-scrolling dogfights. Coordinates are relative to a camera following
// the player's horizontal flight; this avoids unbounded world coordinates.
class SkyPatrol {
 public:
  uint32_t score = 0;
  int lives = 3, kills = 0;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; kills = 0; lives = hard ? 2 : 3;
    over = started = controlsReady = shotQueued = boosting = false;
    scenery = 0; frames = 0;
    respawn();
  }

  // Absolute pointing: atan2 maps the two joystick axes to a target heading.
  // GPIO13 is boost, GPIO14 is the gun (both may be held together).
  void update(int sx, int sy, bool boostHeld, bool shootHeld) {
    if (over) return;
    if (!started) {
      if (boostHeld) { started = true; lastFrame = millis(); }
      return;
    }
    if (!boostHeld && !shootHeld) controlsReady = true;
    if (controlsReady) shotQueued |= shootHeld;
    uint32_t now = millis();
    if (now - lastFrame < 20) return;
    lastFrame = now; ++frames;
    if (immune) --immune;
    if (gunCooldown) --gunCooldown;
    if (abs(sx) > 650 || abs(sy) > 650) {
      float target = atan2f(-float(sy), float(sx));
      angle = wrapAngle(angle + constrain(wrapAngle(target - angle), -0.12f, 0.12f));
    }
    boosting = controlsReady && boostHeld;
    float pace = boosting ? 1.65f : 0.85f;
    scroll = cosf(angle) * pace;
    y += sinf(angle) * pace;
    y = max(y, 14.0f); // The top of the display is the flight ceiling.
    if (y + 3 >= WATER) { loseLife(); return; } // Shields never prevent a sea crash.
    scenery = wrapPhase(scenery + scroll, 192);
    if (shotQueued && !gunCooldown && shoot(PLAYER_X, y, angle, true)) gunCooldown = 8;
    shotQueued = false;

    for (auto &e : enemies) {
      if (!e.hp) continue;
      float target = atan2f(y - e.y, PLAYER_X - e.x);
      float error = wrapAngle(target - e.angle);
      float rate = hard ? 0.025f : 0.017f;
      e.angle = wrapAngle(e.angle + constrain(error, -rate, rate));
      float speed = min((hard ? 0.72f : 0.56f) + wave() * 0.025f, 1.0f);
      e.x += cosf(e.angle) * speed - scroll;
      e.y = constrain(e.y + sinf(e.angle) * speed, 16.0f, 48.0f);
      if (e.x < -45 || e.x > 173) { e.hp = 0; continue; }
      if (e.cooldown) --e.cooldown;
      if (!e.cooldown && e.x > -5 && e.x < 133 && abs(error) < 0.3f) {
        if (shoot(e.x, e.y, e.angle, false)) e.cooldown = hard ? 85 : 135;
      }
      if (!immune && distance2(PLAYER_X, y, e.x, e.y) < 49) { loseLife(); return; }
    }
    if (updateShots()) return;
    if (spawnTimer) --spawnTimer;
    if (!spawnTimer) spawn();
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextColor(SSD1306_WHITE); d.setTextSize(1);
    if (!started) {
      line(d, 0, "SKY PATROL"); line(d, 12, "Point stick to steer");
      line(d, 23, "14 fire / 13 boost"); line(d, 34, "Shoot enemy aircraft");
      line(d, 45, "Stay above the sea!"); line(d, 56, "13: take off");
      d.display(); return;
    }
    for (int i = 0; i < 3; ++i) {
      int cx = int(wrapPhase(i * 64 - scenery * 0.5f, 192)) - 20;
      d.drawFastHLine(cx, 18 + (i % 2) * 9, 19, SSD1306_WHITE);
      d.drawFastHLine(cx + 4, 15 + (i % 2) * 9, 10, SSD1306_WHITE);
      d.drawPixel(cx + 3, 17 + (i % 2) * 9, SSD1306_WHITE);
      d.drawPixel(cx + 15, 17 + (i % 2) * 9, SSD1306_WHITE);
    }
    for (int px = -int(scenery) % 12 - 12; px < 128; px += 12) {
      d.drawLine(px, WATER + 1, px + 3, WATER, SSD1306_WHITE);
      d.drawLine(px + 3, WATER, px + 6, WATER + 1, SSD1306_WHITE);
      d.drawFastHLine(px + 6, WATER + 1, 6, SSD1306_WHITE);
      d.drawFastHLine(px + (frames / 8) % 4, 61, 5, SSD1306_WHITE);
    }
    for (auto &e : enemies) if (e.hp) plane(d, e.x, e.y, e.angle, false);
    if (!immune || frames % 10 < 5) {
      plane(d, PLAYER_X, y, angle, true);
      if (boosting) d.drawLine(int(PLAYER_X - cosf(angle) * 6), int(y - sinf(angle) * 6),
        int(PLAYER_X - cosf(angle) * (frames % 4 < 2 ? 12 : 9)),
        int(y - sinf(angle) * (frames % 4 < 2 ? 12 : 9)), SSD1306_WHITE);
    }
    for (auto &s : shots) if (s.life) {
      if (s.friendly) d.drawLine(int(s.x), int(s.y), int(s.x - s.vx), int(s.y - s.vy), SSD1306_WHITE);
      else d.drawCircle(int(s.x), int(s.y), 1, SSD1306_WHITE);
    }
    d.fillRect(0, 0, 128, 10, SSD1306_BLACK);
    d.setCursor(0, 0); d.print(score);
    d.setCursor(43, 0); d.print(F("L")); d.print(lives);
    d.setCursor(62, 0); d.print(F("W")); d.print(wave());
    d.setCursor(96, 0); d.print(boosting ? F("BOOST") : hard ? F("HARD") : F("EASY"));
    d.drawFastHLine(0, 9, 128, SSD1306_WHITE); d.display();
  }

 private:
  static constexpr float PLAYER_X = 38, PI_VALUE = 3.14159265f;
  static constexpr int WATER = 56;
  struct Enemy { float x, y, angle; int hp, cooldown; };
  struct Shot { float x, y, vx, vy; int life; bool friendly; };
  Enemy enemies[3] = {};
  Shot shots[20] = {};
  float y = 28, angle = 0, scroll = 0, scenery = 0;
  int immune = 0, gunCooldown = 0, spawnTimer = 0;
  uint32_t lastFrame = 0, frames = 0;
  bool hard = false, started = false, controlsReady = false, shotQueued = false, boosting = false;

  int wave() const { return min(1 + kills / 5, 99); }
  static float wrapAngle(float a) {
    while (a > PI_VALUE) a -= 2 * PI_VALUE;
    while (a < -PI_VALUE) a += 2 * PI_VALUE;
    return a;
  }
  static float wrapPhase(float a, float limit) {
    while (a < 0) a += limit;
    while (a >= limit) a -= limit;
    return a;
  }
  static float distance2(float ax, float ay, float bx, float by) {
    float dx = ax - bx, dy = ay - by; return dx * dx + dy * dy;
  }
  bool shoot(float x, float sy, float a, bool friendly) {
    for (auto &s : shots) if (!s.life) {
      float pace = friendly ? 3.2f : (hard ? 1.9f : 1.55f);
      s = {x + cosf(a) * 7, sy + sinf(a) * 7,
           cosf(a) * pace, sinf(a) * pace, 65, friendly};
      return true;
    }
    return false;
  }
  bool updateShots() {
    for (auto &s : shots) {
      if (!s.life) continue;
      if (--s.life == 0) continue;
      for (int step = 0; step < 5 && s.life; ++step) {
        s.x += (s.vx - scroll) / 5; s.y += s.vy / 5;
        if (s.x < -30 || s.x > 158 || s.y < 10 || s.y >= WATER) { s.life = 0; break; }
        if (s.friendly) {
          for (auto &e : enemies) if (e.hp && distance2(s.x, s.y, e.x, e.y) < 25) {
            s.life = 0;
            if (--e.hp == 0) { kills = min(kills + 1, 9999); score = min(score + 100, uint32_t(999999)); }
            break;
          }
        } else if (distance2(s.x, s.y, PLAYER_X, y) < 16) {
          s.life = 0;
          if (!immune) { loseLife(); return true; }
        }
      }
    }
    return false;
  }
  void spawn() {
    int limit = hard || wave() >= 3 ? 3 : 2;
    int active = 0; for (auto &e : enemies) active += e.hp > 0;
    if (active >= limit) { spawnTimer = 15; return; }
    for (auto &e : enemies) if (!e.hp) {
      bool right = cosf(angle) >= 0;
      float ex = right ? 140 : -14, ey = float(random(20, 45));
      e = {ex, ey, right ? PI_VALUE : 0, hard ? 2 : 1, hard ? 60 : 100};
      spawnTimer = max(60, (hard ? 110 : 150) - wave() * 6);
      return;
    }
  }
  void respawn() {
    y = 28; angle = scroll = 0;
    immune = 100; gunCooldown = 0; spawnTimer = 45;
    shotQueued = boosting = false;
    for (auto &e : enemies) e = {};
    for (auto &s : shots) s = {};
    lastFrame = millis();
  }
  void loseLife() {
    if (--lives <= 0) { lives = 0; over = true; return; }
    respawn();
  }
  static void plane(Adafruit_SSD1306 &d, float x, float y, float a, bool player) {
    float cs = cosf(a), sn = sinf(a);
    d.drawLine(int(x - cs * 5), int(y - sn * 5), int(x + cs * 7), int(y + sn * 7), SSD1306_WHITE);
    d.drawLine(int(x - cs * 2 - sn * 5), int(y - sn * 2 + cs * 5),
               int(x + cs * 3), int(y + sn * 3), SSD1306_WHITE);
    d.drawLine(int(x - cs * 2 + sn * 5), int(y - sn * 2 - cs * 5),
               int(x + cs * 3), int(y + sn * 3), SSD1306_WHITE);
    d.drawLine(int(x - cs * 5 - sn * 2), int(y - sn * 5 + cs * 2),
               int(x - cs * 5 + sn * 2), int(y - sn * 5 - cs * 2), SSD1306_WHITE);
    if (player) d.fillCircle(int(x), int(y), 1, SSD1306_WHITE);
    else d.drawCircle(int(x), int(y), 2, SSD1306_WHITE);
  }
  static void line(Adafruit_SSD1306 &d, int y, const char *s) { d.setCursor(0, y); d.print(s); }
};
