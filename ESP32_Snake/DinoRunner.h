#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Original OLED runner inspired by the offline dinosaur game.
class DinoRunner {
 public:
  uint32_t score = 0;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; over = started = false;
    feet = GROUND; velocity = distance = groundOffset = 0;
    grounded = true; ducking = jumpQueued = stickReady = false;
    spawnDistance = 80;
    for (auto &o : obstacles) o = {};
    lastFrame = millis(); frames = 0;
  }

  void update(int stickY, bool jumpPressed, bool duckHeld) {
    if (over) return;
    if (stickY < 350) stickReady = true;
    bool stickJump = stickReady && stickY > 650;
    if (stickJump) stickReady = false;
    bool jump = jumpPressed || stickJump;
    if (!started) {
      if (jump) { started = true; lastFrame = millis(); }
      return; // Starting the run never spends its first jump.
    }
    jumpQueued |= jump;
    uint32_t now = millis();
    if (now - lastFrame < 20) return;
    lastFrame = now; ++frames;
    bool duck = duckHeld || stickY < -650;
    ducking = grounded && duck;
    if (jumpQueued && grounded && !duck) {
      velocity = -3.8f; grounded = false; ducking = false;
    }
    jumpQueued = false; // No midair jump or automatic jump on landing.
    if (!grounded) {
      velocity += 0.24f;
      feet += velocity;
      if (feet >= GROUND) { feet = GROUND; velocity = 0; grounded = true; ducking = duck; }
    }
    float pace = speed();
    groundOffset += pace;
    if (groundOffset >= 16) groundOffset -= 16;
    for (auto &o : obstacles) {
      if (!o.active) continue;
      o.x -= pace;
      if (o.x + o.w < 0) { o.active = false; continue; }
      // Inset hitboxes forgive the transparent corners of the small sprites.
      float px = PLAYER_X + 2, py = feet - (ducking ? 8 : 16) + 2;
      int pw = ducking ? 15 : 9, ph = ducking ? 6 : 14;
      if (px < o.x + o.w - 1 && px + pw > o.x + 1 &&
          py < o.y + o.h - 1 && py + ph > o.y + 1) { over = true; return; }
    }
    distance += pace;
    while (distance >= 8) { distance -= 8; if (score < 999999) ++score; }
    spawnDistance -= pace;
    if (spawnDistance <= 0) spawn();
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    if (!started) {
      line(d, 0, "DINO RUNNER"); line(d, 12, "13 / UP: jump");
      line(d, 23, "14 / DOWN: duck"); line(d, 34, "Dodge cacti + birds");
      line(d, 45, "Run gets faster!"); line(d, 56, "13: start running");
      d.display(); return;
    }
    d.setCursor(0, 0); d.print(F("RUN ")); d.print(score);
    d.setCursor(100, 0); d.print(hard ? F("HARD") : F("EASY"));
    // Clouds drift more slowly than the ground.
    for (int i = 0; i < 2; ++i) {
      int cx = 145 - int((frames / 4 + i * 75) % 170);
      d.drawFastHLine(cx, 21 + i * 5, 15, SSD1306_WHITE);
      d.drawFastHLine(cx + 4, 18 + i * 5, 7, SSD1306_WHITE);
      d.drawPixel(cx + 3, 20 + i * 5, SSD1306_WHITE);
      d.drawPixel(cx + 11, 20 + i * 5, SSD1306_WHITE);
    }
    d.drawFastHLine(0, GROUND, 128, SSD1306_WHITE);
    for (int x = -int(groundOffset); x < 128; x += 16) {
      d.drawFastHLine(x, 61, 4, SSD1306_WHITE); d.drawPixel(x + 10, 60, SSD1306_WHITE);
    }
    for (auto &o : obstacles) if (o.active) {
      int x = int(o.x), y = o.y;
      if (o.kind >= 3) {
        d.fillRect(x + 3, y + 3, 8, 3, SSD1306_WHITE);
        d.drawLine(x, y + 3, x + 3, y + 2, SSD1306_WHITE);
        int wing = frames % 16 < 8 ? y : y + 7;
        d.drawLine(x + 6, y + 4, x + 9, wing, SSD1306_WHITE);
        d.drawLine(x + 9, wing, x + 12, y + 4, SSD1306_WHITE);
      } else {
        cactus(d, x, y, o.kind == 2 ? 7 : o.w, o.h);
        if (o.kind == 2) cactus(d, x + 9, y + 2, 7, o.h - 2);
      }
    }
    int x = PLAYER_X, y = int(feet) - (ducking ? 8 : 16);
    int stride = grounded && frames % 12 < 6 ? 2 : 0;
    if (ducking) {
      d.fillRect(x + 2, y + 2, 10, 4, SSD1306_WHITE);
      d.fillRect(x + 11, y, 7, 5, SSD1306_WHITE);
      d.drawPixel(x + 15, y + 1, SSD1306_BLACK);
      d.drawLine(x, y, x + 3, y + 3, SSD1306_WHITE);
      d.drawFastHLine(x + 3 + stride, y + 7, 3, SSD1306_WHITE);
      d.drawFastHLine(x + 9 - stride, y + 7, 3, SSD1306_WHITE);
    } else {
      d.fillRect(x + 5, y, 8, 6, SSD1306_WHITE);
      d.drawPixel(x + 10, y + 1, SSD1306_BLACK);
      d.drawFastHLine(x + 9, y + 4, 4, SSD1306_BLACK);
      d.fillRect(x + 4, y + 5, 5, 7, SSD1306_WHITE);
      d.fillRect(x + 2, y + 8, 4, 5, SSD1306_WHITE);
      d.drawLine(x, y + 6, x + 3, y + 10, SSD1306_WHITE);
      d.drawFastHLine(x + 8, y + 8, 3, SSD1306_WHITE);
      d.drawFastVLine(x + 3, y + 12, 4 - stride, SSD1306_WHITE);
      d.drawFastVLine(x + 7, y + 12, 2 + stride, SSD1306_WHITE);
      d.drawPixel(x + 4, y + 15 - stride, SSD1306_WHITE);
      d.drawPixel(x + 8, y + 13 + stride, SSD1306_WHITE);
    }
    d.display();
  }

 private:
  static constexpr int GROUND = 58, PLAYER_X = 18;
  struct Obstacle { float x; int y, w, h, kind; bool active; };
  Obstacle obstacles[4] = {};
  bool hard = false, started = false, grounded = true, ducking = false;
  bool jumpQueued = false, stickReady = false;
  float feet = GROUND, velocity = 0, distance = 0, groundOffset = 0, spawnDistance = 80;
  uint32_t lastFrame = 0, frames = 0;

  float speed() const { return min((hard ? 1.9f : 1.5f) + score * 0.0015f, hard ? 2.9f : 2.6f); }
  void spawn() {
    for (auto &o : obstacles) if (!o.active) {
      int kind = random(score >= 150 ? 5 : 3);
      int w = kind >= 3 ? 14 : kind == 2 ? 16 : kind == 1 ? 8 : 7;
      int h = kind >= 3 ? 8 : kind == 1 ? 16 : 11;
      int y = kind == 3 ? 39 : kind == 4 ? 48 : GROUND - h;
      o = {136, y, w, h, kind, true};
      // A full jump takes 31 frames. Keep enough time to land and react to
      // the next hazard at every speed, including the wider ducking hitbox.
      spawnDistance = w + speed() * (hard ? 48 : 58) + random(20, 51);
      return;
    }
  }
  static void cactus(Adafruit_SSD1306 &d, int x, int y, int w, int h) {
    d.fillRect(x + w / 2 - 1, y, 3, h, SSD1306_WHITE);
    d.drawFastVLine(x, y + 3, h / 2, SSD1306_WHITE);
    d.drawFastHLine(x, y + 3 + h / 2, w / 2, SSD1306_WHITE);
    d.drawFastVLine(x + w - 1, y + 1, h / 2, SSD1306_WHITE);
    d.drawFastHLine(x + w / 2, y + 1 + h / 2, w / 2, SSD1306_WHITE);
  }
  static void line(Adafruit_SSD1306 &d, int y, const char *s) { d.setCursor(0, y); d.print(s); }
};
