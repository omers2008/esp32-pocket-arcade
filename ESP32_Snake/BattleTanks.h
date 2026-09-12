#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

class BattleTanks {
 public:
  uint32_t score = 0;
  bool over = false, won = false;
  int hp = 5, wave = 1;

  void start(bool hardMode) {
    hard = hardMode; score = 0; hp = hard ? 3 : 5; wave = 1;
    over = won = started = false;
    fireReady = normalQueued = bounceQueued = false;
    loadWave();
  }

  // X turns the hull/barrel. Y drives along that heading (down reverses).
  // Held buttons repeat; short presses survive the 20 ms physics interval.
  void update(int sx, int sy, bool normalHeld, bool bounceHeld) {
    if (over) return;
    if (!started) {
      if (normalHeld) { started = true; lastFrame = millis(); }
      return;
    }
    if (!normalHeld && !bounceHeld) fireReady = true;
    if (fireReady) { normalQueued |= normalHeld; bounceQueued |= bounceHeld; }
    uint32_t now = millis();
    if (now - lastFrame < 20) return;
    lastFrame = now;
    if (immune) --immune;
    if (cooldown) --cooldown;
    int turn = sx > 650 ? 1 : sx < -650 ? -1 : 0;
    int drive = sy > 650 ? 1 : sy < -650 ? -1 : 0;
    player.angle = wrap(player.angle + turn * 0.065f);
    moveTank(player, drive * (drive < 0 ? 0.55f : 0.8f), -1);
    if (!cooldown && (normalQueued || bounceQueued)) {
      if (fire(player, true, bounceQueued)) cooldown = bounceQueued ? 25 : 13;
    }
    normalQueued = bounceQueued = false;
    updateCPUs();
    updateShells();
    if (over) return;
    bool cleared = true;
    for (auto &c : cpus) if (c.hp > 0) cleared = false;
    if (cleared) {
      score += 200;
      if (wave == 5) { score += hp * 100; won = over = true; }
      else { ++wave; hp = min(hp + 1, hard ? 3 : 5); loadWave(); }
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    if (!started) {
      line(d, 0, "BATTLE TANKS"); line(d, 11, "L/R turn, U/D drive");
      line(d, 22, "13 shell 14 ricochet"); line(d, 33, "4 bounces. Dodge ALL");
      line(d, 44, "returning shots!"); line(d, 55, "13 start / 5 waves");
      d.display(); return;
    }
    d.drawRect(0, 10, 128, 54, SSD1306_WHITE);
    for (int i = 0; i < 2; ++i) {
      Wall w = wall(i);
      d.drawRect(w.x, w.y, w.w, w.h, SSD1306_WHITE);
      d.drawLine(w.x, w.y, w.x + w.w - 1, w.y + w.h - 1, SSD1306_WHITE);
    }
    for (auto &c : cpus) if (c.hp > 0) drawTank(d, c, false);
    if (!immune || immune % 10 < 5) drawTank(d, player, true);
    for (auto &s : shells) if (s.active) {
      if (s.bouncy) d.drawCircle(int(s.x), int(s.y), 1, SSD1306_WHITE);
      else d.fillRect(int(s.x), int(s.y), 2, 2, SSD1306_WHITE);
    }
    d.setCursor(0, 0); d.print(F("HP")); d.print(hp);
    d.setCursor(28, 0); d.print(F("W")); d.print(wave); d.print(F("/5"));
    d.setCursor(61, 0); d.print(score);
    d.setCursor(115, 0); d.print(hard ? 'H' : 'E');
    d.display();
  }

 private:
  struct Tank { float x, y, angle; int hp, cooldown, detour; };
  struct Wall { int x, y, w, h; };
  struct Shell {
    float x, y, vx, vy;
    int bounces, life;
    bool active, playerOwned, bouncy;
  };
  Tank player = {}, cpus[3] = {};
  Shell shells[24] = {};
  bool hard = false, started = false, fireReady = false;
  bool normalQueued = false, bounceQueued = false;
  int immune = 0, cooldown = 0;
  uint32_t lastFrame = 0;
  static constexpr float PI_VALUE = 3.14159265f;

  static Wall wall(int i) { return i == 0 ? Wall{43, 22, 9, 17} : Wall{77, 36, 9, 17}; }
  static float wrap(float a) {
    while (a > PI_VALUE) a -= 2 * PI_VALUE;
    while (a < -PI_VALUE) a += 2 * PI_VALUE;
    return a;
  }
  static float distance2(float ax, float ay, float bx, float by) {
    float dx = ax - bx, dy = ay - by; return dx * dx + dy * dy;
  }
  static bool solid(float x, float y, float radius) {
    if (x - radius <= 0 || x + radius >= 127 || y - radius <= 10 || y + radius >= 63) return true;
    for (int i = 0; i < 2; ++i) {
      Wall w = wall(i);
      if (x + radius >= w.x && x - radius <= w.x + w.w - 1 &&
          y + radius >= w.y && y - radius <= w.y + w.h - 1) return true;
    }
    return false;
  }
  bool moveTank(Tank &t, float speed, int cpuIndex) {
    float nx = t.x + cosf(t.angle) * speed, ny = t.y + sinf(t.angle) * speed;
    if (solid(nx, ny, 4)) return false;
    if (cpuIndex >= 0 && distance2(nx, ny, player.x, player.y) < 81) return false;
    for (int i = 0; i < 3; ++i)
      if (i != cpuIndex && cpus[i].hp > 0 && distance2(nx, ny, cpus[i].x, cpus[i].y) < 81) return false;
    t.x = nx; t.y = ny; return true;
  }
  static bool clearSight(const Tank &a, const Tank &b) {
    float dx = b.x - a.x, dy = b.y - a.y;
    int steps = int(max(abs(dx), abs(dy))) + 1;
    for (int i = 1; i < steps; ++i)
      if (solid(a.x + dx * i / steps, a.y + dy * i / steps, 1)) return false;
    return true;
  }
  bool fire(const Tank &t, bool playerOwned, bool bouncy) {
    for (auto &s : shells) if (!s.active) {
      float vx = cosf(t.angle), vy = sinf(t.angle);
      // Spawn inside the tank radius; collision ignores the shooter until a bounce.
      // Starting here also prevents a barrel beside cover firing through it.
      s = {t.x + vx * 3, t.y + vy * 3, vx * 2.2f, vy * 2.2f,
           0, 900, true, playerOwned, bouncy};
      return true;
    }
    return false;
  }
  void updateCPUs() {
    for (int i = 0; i < 3; ++i) {
      Tank &c = cpus[i]; if (c.hp <= 0) continue;
      if (c.cooldown) --c.cooldown;
      if (c.detour) {
        --c.detour;
        c.angle = wrap(c.angle + (i % 2 ? -0.055f : 0.055f));
        moveTank(c, hard ? 0.5f : 0.36f, i);
        continue;
      }
      float target = atan2f(player.y - c.y, player.x - c.x);
      float error = wrap(target - c.angle);
      float rate = hard ? 0.045f : 0.03f;
      c.angle = wrap(c.angle + constrain(error, -rate, rate));
      bool sight = clearSight(c, player);
      if (abs(error) < 0.4f && (!sight || distance2(c.x, c.y, player.x, player.y) > 900)) {
        if (!moveTank(c, hard ? 0.48f : 0.32f, i)) c.detour = 45 + i * 10;
      }
      if (sight && abs(wrap(target - c.angle)) < 0.14f && !c.cooldown) {
        if (fire(c, false, false)) c.cooldown = (hard ? 85 : 140) - wave * 5 + i * 8;
      }
    }
  }
  void updateShells() {
    for (auto &s : shells) {
      if (!s.active) continue;
      if (--s.life <= 0) { s.active = false; continue; }
      // Subpixel steps prevent skipping thin cover or a tank at shell speed.
      for (int step = 0; step < 5 && s.active; ++step) {
        float nx = s.x + s.vx / 5, ny = s.y + s.vy / 5;
        if (solid(nx, ny, 1)) {
          if (!s.bouncy || s.bounces == 4) { s.active = false; break; }
          bool hitX = solid(nx, s.y, 1), hitY = solid(s.x, ny, 1);
          // A corner hit is one impact even if it reflects both axes.
          if (hitX || (!hitX && !hitY)) s.vx = -s.vx;
          if (hitY || (!hitX && !hitY)) s.vy = -s.vy;
          ++s.bounces;
        } else { s.x = nx; s.y = ny; }
        if ((!s.playerOwned || s.bounces > 0) && distance2(s.x, s.y, player.x, player.y) < 25) {
          s.active = false;
          if (!immune) { --hp; immune = 50; if (hp == 0) over = true; }
        }
        if (s.active && s.playerOwned) for (auto &c : cpus) {
          if (c.hp > 0 && distance2(s.x, s.y, c.x, c.y) < 25) {
            s.active = false;
            if (--c.hp == 0) score += 100;
            break;
          }
        }
      }
    }
  }
  void loadWave() {
    player = {16, 37, 0, 1, 0, 0};
    cpus[0] = {111, 20, PI_VALUE, 2, 100, 0};
    cpus[1] = {111, 54, PI_VALUE, 2, 140, 0};
    cpus[2] = {65, 20, PI_VALUE / 2, wave >= 3 ? 2 : 0, 180, 0};
    for (auto &s : shells) s = {};
    immune = 75; cooldown = 0; normalQueued = bounceQueued = false;
    lastFrame = millis();
  }
  static void drawTank(Adafruit_SSD1306 &d, const Tank &t, bool filled) {
    float cs = cosf(t.angle), sn = sinf(t.angle);
    int px[4], py[4];
    const int forward[] = {-4, 4, 4, -4}, side[] = {-3, -3, 3, 3};
    for (int i = 0; i < 4; ++i) {
      px[i] = int(t.x + cs * forward[i] - sn * side[i]);
      py[i] = int(t.y + sn * forward[i] + cs * side[i]);
    }
    for (int i = 0; i < 4; ++i) d.drawLine(px[i], py[i], px[(i + 1) % 4], py[(i + 1) % 4], SSD1306_WHITE);
    if (filled) d.fillCircle(int(t.x), int(t.y), 2, SSD1306_WHITE);
    else d.drawCircle(int(t.x), int(t.y), 1, SSD1306_WHITE);
    d.drawLine(int(t.x), int(t.y), int(t.x + cs * 7), int(t.y + sn * 7), SSD1306_WHITE);
  }
  static void line(Adafruit_SSD1306 &d, int y, const char *s) { d.setCursor(0, y); d.print(s); }
};
