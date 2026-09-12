#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// Original monochrome space shooter with a toroidal (wrapping) playfield.
class Asteroids {
 public:
  uint32_t score = 0;
  bool over = false;
  int lives = 3, wave = 1;

  void start(bool hardMode) {
    hard = hardMode; score = 0; lives = hard ? 2 : 3; wave = 1;
    over = started = fireReady = fireQueued = hyperQueued = thrusting = false;
    frames = 0; loadWave();
  }

  void update(int sx, int sy, bool fireHeld, bool hyperPressed) {
    if (over) return;
    if (!started) {
      if (fireHeld) { started = true; lastFrame = millis(); }
      return;
    }
    if (!fireHeld) fireReady = true;
    if (fireReady) fireQueued |= fireHeld;
    hyperQueued |= hyperPressed;
    uint32_t now = millis();
    if (now - lastFrame < 20) return;
    lastFrame = now; ++frames;
    if (immune) --immune;
    if (fireCooldown) --fireCooldown;
    if (hyperCooldown) --hyperCooldown;
    if (hyperQueued && !hyperCooldown) {
      relocate(); immune = 35; hyperCooldown = 200;
    }
    hyperQueued = false;
    int turn = sx > 650 ? 1 : sx < -650 ? -1 : 0;
    angle += turn * 0.085f;
    if (angle > PI_VALUE) angle -= 2 * PI_VALUE;
    if (angle < -PI_VALUE) angle += 2 * PI_VALUE;
    thrusting = sy > 650;
    if (thrusting) { vx += cosf(angle) * 0.065f; vy += sinf(angle) * 0.065f; }
    float speed2 = vx * vx + vy * vy;
    if (speed2 > 2.89f) { float scale = 1.7f / sqrtf(speed2); vx *= scale; vy *= scale; }
    x = wrap(x + vx, WIDTH); y = wrap(y + vy, HEIGHT);
    vx *= 0.995f; vy *= 0.995f;
    if (fireQueued && !fireCooldown && fire()) fireCooldown = 9;
    fireQueued = false;
    for (auto &r : rocks) if (r.size) {
      r.x = wrap(r.x + r.vx, WIDTH); r.y = wrap(r.y + r.vy, HEIGHT);
    }
    updateShots();
    if (!immune) for (auto &r : rocks) if (r.size && touches(x, y, r.x, r.y, radius(r.size) + 2.5f)) {
      if (--lives == 0) { over = true; return; }
      relocate(); immune = 100; fireCooldown = 12;
      for (auto &s : shots) s.life = 0;
      break;
    }
    bool cleared = true;
    for (auto &r : rocks) if (r.size) cleared = false;
    if (cleared) { addScore(200); wave = min(wave + 1, 999); loadWave(); }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextColor(SSD1306_WHITE); d.setTextSize(1);
    if (!started) {
      line(d, 0, "ASTEROIDS"); line(d, 12, "L/R turn, UP thrust");
      line(d, 23, "13 shoot (hold)"); line(d, 34, "14 hyperspace");
      line(d, 45, "Split rocks + survive"); line(d, 56, "13: launch");
      d.display(); return;
    }
    // Render copies at seams so wrapping bodies do not pop out of view.
    for (auto &r : rocks) if (r.size) for (int ox = -128; ox <= 128; ox += 128)
      for (int oy = -54; oy <= 54; oy += 54) {
        float cx = r.x + ox, cy = r.y + oy; float rad = radius(r.size);
        if (cx + rad < 0 || cx - rad > 127 || cy + rad < 0 || cy - rad > 53) continue;
        const float shape[] = {1, 0.8f, 1, 0.72f, 0.95f, 0.8f, 1, 0.85f};
        for (int i = 0; i < 8; ++i) {
          int j = (i + 1) % 8;
          float a = i * PI_VALUE / 4, b = j * PI_VALUE / 4;
          d.drawLine(int(cx + cosf(a) * rad * shape[i]), int(cy + sinf(a) * rad * shape[i]) + 10,
                     int(cx + cosf(b) * rad * shape[j]), int(cy + sinf(b) * rad * shape[j]) + 10, SSD1306_WHITE);
        }
      }
    for (auto &s : shots) if (s.life) d.drawPixel(int(s.x), int(s.y) + 10, SSD1306_WHITE);
    if (!immune || frames % 10 < 5) for (int ox = -128; ox <= 128; ox += 128)
      for (int oy = -54; oy <= 54; oy += 54) {
        float cx = x + ox, cy = y + oy;
        if (cx < -7 || cx > 134 || cy < -7 || cy > 60) continue;
        int nx = int(cx + cosf(angle) * 6), ny = int(cy + sinf(angle) * 6) + 10;
        int lx = int(cx + cosf(angle + 2.5f) * 5), ly = int(cy + sinf(angle + 2.5f) * 5) + 10;
        int rx = int(cx + cosf(angle - 2.5f) * 5), ry = int(cy + sinf(angle - 2.5f) * 5) + 10;
        d.drawLine(nx, ny, lx, ly, SSD1306_WHITE); d.drawLine(nx, ny, rx, ry, SSD1306_WHITE);
        d.drawLine(lx, ly, int(cx), int(cy) + 10, SSD1306_WHITE);
        d.drawLine(rx, ry, int(cx), int(cy) + 10, SSD1306_WHITE);
        if (thrusting && frames % 4 < 2)
          d.drawLine(int(cx - cosf(angle) * 3), int(cy - sinf(angle) * 3) + 10,
                     int(cx - cosf(angle) * 8), int(cy - sinf(angle) * 8) + 10, SSD1306_WHITE);
      }
    d.fillRect(0, 0, 128, 10, SSD1306_BLACK);
    d.setCursor(0, 0); d.print(score);
    d.setCursor(40, 0); d.print(F("L")); d.print(lives);
    d.setCursor(58, 0); d.print(F("W")); d.print(wave);
    d.setCursor(89, 0); d.print(F("H")); d.print((hyperCooldown + 49) / 50);
    d.setCursor(116, 0); d.print(hard ? 'H' : 'E');
    d.drawFastHLine(0, 9, 128, SSD1306_WHITE); d.display();
  }

 private:
  static constexpr float WIDTH = 128, HEIGHT = 54, PI_VALUE = 3.14159265f;
  struct Rock { float x, y, vx, vy; int size; };
  struct Shot { float x, y, vx, vy; int life; };
  Rock rocks[24] = {};
  Shot shots[8] = {};
  float x = 64, y = 27, vx = 0, vy = 0, angle = -PI_VALUE / 2;
  bool hard = false, started = false, fireReady = false, fireQueued = false;
  bool hyperQueued = false, thrusting = false;
  int immune = 0, fireCooldown = 0, hyperCooldown = 0;
  uint32_t lastFrame = 0, frames = 0;

  static float wrap(float value, float bound) {
    while (value < 0) value += bound;
    while (value >= bound) value -= bound;
    return value;
  }
  static float dist2(float ax, float ay, float bx, float by) {
    float dx = abs(ax - bx), dy = abs(ay - by);
    dx = min(dx, WIDTH - dx); dy = min(dy, HEIGHT - dy);
    return dx * dx + dy * dy;
  }
  static bool touches(float ax, float ay, float bx, float by, float r) { return dist2(ax, ay, bx, by) < r * r; }
  static float radius(int size) { return size == 3 ? 8.0f : size == 2 ? 5.0f : 2.5f; }
  void addScore(uint32_t n) { score = min(score + n, uint32_t(999999)); }
  bool fire() {
    for (auto &s : shots) if (!s.life) {
      s = {wrap(x + cosf(angle) * 5, WIDTH), wrap(y + sinf(angle) * 5, HEIGHT),
           cosf(angle) * 2.8f + vx * 0.4f, sinf(angle) * 2.8f + vy * 0.4f, 45};
      return true;
    }
    return false;
  }
  void updateShots() {
    for (auto &s : shots) {
      if (!s.life) continue;
      if (--s.life == 0) continue;
      for (int step = 0; step < 4 && s.life; ++step) {
        s.x = wrap(s.x + s.vx / 4, WIDTH); s.y = wrap(s.y + s.vy / 4, HEIGHT);
        for (int i = 0; i < 24; ++i) if (rocks[i].size && touches(s.x, s.y, rocks[i].x, rocks[i].y, radius(rocks[i].size) + 0.5f)) {
          s.life = 0; split(i); break;
        }
      }
    }
  }
  void split(int index) {
    Rock old = rocks[index];
    if (!old.size) return;
    rocks[index].size = 0;
    addScore(old.size == 3 ? 20 : old.size == 2 ? 50 : 100);
    if (old.size == 1) return;
    // Five large rocks produce at most twenty live fragments; the 24-slot
    // pool always has room for both children, including the vacated slot.
    float direction = atan2f(old.vy, old.vx);
    float pace = min(sqrtf(old.vx * old.vx + old.vy * old.vy) + 0.12f, 0.95f);
    int children = 0;
    for (auto &r : rocks) if (!r.size) {
      float a = direction + (children == 0 ? -0.75f : 0.75f);
      r = {old.x, old.y, cosf(a) * pace, sinf(a) * pace, old.size - 1};
      if (++children == 2) break;
    }
  }
  void relocate() {
    // Pick the clearest of 24 candidate positions, well away from the old
    // ship when possible; a brief shield covers unavoidable dense fields.
    float best = -1, bestX = 64, bestY = 27;
    for (int i = 0; i < 24; ++i) {
      float cx = random(128), cy = random(54), clearance = 10000;
      if (touches(cx, cy, x, y, 16)) continue;
      for (auto &r : rocks) if (r.size)
        clearance = min(clearance, sqrtf(dist2(cx, cy, r.x, r.y)) - radius(r.size));
      if (clearance > best) { best = clearance; bestX = cx; bestY = cy; }
    }
    x = bestX; y = bestY; vx = vy = 0;
  }
  void loadWave() {
    for (auto &r : rocks) r = {};
    for (auto &s : shots) s = {};
    int count = min((hard ? 3 : 2) + (wave - 1) / 2, 5);
    float pace = min((hard ? 0.38f : 0.25f) + (wave - 1) * 0.025f, 0.65f);
    for (int i = 0; i < count; ++i) {
      float a = random(628) / 100.0f;
      rocks[i] = {float(random(128)), float(random(54)), cosf(a) * pace, sinf(a) * pace, 3};
    }
    relocate(); angle = -PI_VALUE / 2;
    immune = 100; fireCooldown = hyperCooldown = 0;
    fireQueued = hyperQueued = false; lastFrame = millis();
  }
  static void line(Adafruit_SSD1306 &d, int y, const char *s) { d.setCursor(0, y); d.print(s); }
};
