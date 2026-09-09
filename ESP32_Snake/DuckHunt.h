#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Original monochrome target-shooting game; no original Duck Hunt assets.
class DuckHunt {
 public:
  uint32_t score = 0, round = 1;
  int lives = 5;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; round = 1; completed = 0;
    lives = hard ? 3 : 5; over = false;
    aimX = 64; aimY = 32; shotShown = false;
    lastFrame = millis();
    spawn();
  }

  void update(int stickX, int stickY, bool shootPressed) {
    if (over) return;
    uint32_t now = millis();
    if (phase != FLYING) {
      if (now - resultAt >= 650) {
        if (lives == 0) { over = true; return; }
        ++completed; round = completed / 10 + 1;
        spawn();
      }
      // Result-screen presses never carry into the next target.
      lastFrame = now;
      return;
    }
    if (now - spawnedAt >= timeLimit()) { finish(false); return; }
    // Resolve each debounced edge immediately, even between physics frames.
    // Shoot the current aim position before moving either crosshair or target.
    if (shootPressed) {
      --ammo;
      shotX = aimX; shotY = aimY; shotAt = now; shotShown = true;
      int padding = hard ? 0 : 2;
      if (abs(aimX - duckX) <= 6 + padding && abs(aimY - duckY) <= 4 + padding) {
        score += 100 + ammo * 25;
        finish(true); return;
      }
      if (ammo == 0) { finish(false); return; }
    }
    if (now - lastFrame < 20) return;
    lastFrame = now;
    aimX = constrain(aimX + aimMotion(stickX), 3.0f, 124.0f);
    aimY = constrain(aimY - aimMotion(stickY), 14.0f, 49.0f);
    duckX += vx; duckY += vy;
    if (duckX < 8) { duckX = 16 - duckX; vx = abs(vx); }
    if (duckX > 119) { duckX = 238 - duckX; vx = -abs(vx); }
    if (duckY < 17) { duckY = 34 - duckY; vy = abs(vy); }
    if (duckY > 46) { duckY = 92 - duckY; vy = -abs(vy); }
    if (now - lastTurn >= (hard ? 700u : 1200u)) {
      lastTurn = now;
      if (random(3) == 0) vy = -vy;
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    uint32_t now = millis();
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    d.setCursor(0, 0); d.print(score);
    d.setCursor(66, 0); d.print(F("R")); d.print(round);
    d.setCursor(104, 0); d.print(F("L")); d.print(lives);
    if (phase == FLYING) {
      uint32_t age = min(now - spawnedAt, timeLimit());
      d.drawFastHLine(0, 9, int((timeLimit() - age) * 128 / timeLimit()), SSD1306_WHITE);
    }
    // Low grass, leaving a clean sky for aiming and a separate ammo footer.
    d.drawFastHLine(0, 53, 128, SSD1306_WHITE);
    for (int x = 1; x < 128; x += 9) {
      d.drawLine(x, 53, x + 2, 50, SSD1306_WHITE);
      d.drawLine(x + 3, 53, x + 5, 51, SSD1306_WHITE);
    }
    int dx = int(duckX), dy = int(duckY), facing = vx > 0 ? 1 : -1;
    if (phase == HIT) dy = min(49, dy + int((now - resultAt) / 35));
    if (phase != ESCAPED) {
      d.fillRect(dx - 4, dy - 1, 8, 4, SSD1306_WHITE);
      d.fillRect(dx + (facing > 0 ? 2 : -5), dy - 4, 4, 4, SSD1306_WHITE);
      d.drawPixel(dx + facing * 4, dy - 3, SSD1306_BLACK);
      d.drawPixel(dx + facing * 6, dy - 2, SSD1306_WHITE);
      int flap = (now / 110) % 2 ? -4 : 4;
      d.drawLine(dx, dy, dx - facing * 4, dy + flap, SSD1306_WHITE);
      d.drawPixel(dx - facing * 5, dy + 2, SSD1306_WHITE);
    }
    if (phase == FLYING) {
      int ax = int(aimX), ay = int(aimY);
      // Black outline keeps the reticle visible when it overlaps a white duck.
      d.drawRect(ax - 4, ay - 4, 9, 9, SSD1306_BLACK);
      d.drawLine(ax - 5, ay, ax - 2, ay, SSD1306_WHITE);
      d.drawLine(ax + 2, ay, ax + 5, ay, SSD1306_WHITE);
      d.drawLine(ax, ay - 5, ax, ay - 2, SSD1306_WHITE);
      d.drawLine(ax, ay + 2, ax, ay + 5, SSD1306_WHITE);
      d.drawPixel(ax, ay, SSD1306_WHITE);
      if (shotShown && now - shotAt < 100) {
        d.drawRect(int(shotX) - 6, int(shotY) - 6, 13, 13, SSD1306_WHITE);
      }
    } else {
      d.fillRect(23, 22, 83, 12, SSD1306_BLACK);
      d.setCursor(phase == HIT ? 52 : 31, 24);
      d.print(phase == HIT ? F("HIT!") : F("FLEW AWAY!"));
    }
    d.setCursor(0, 56); d.print(F("AMMO ")); d.print(ammo);
    d.setCursor(48, 56); d.print(hard ? F("H") : F("E"));
    d.setCursor(68, 56); d.print(F("13:SHOT"));
    d.display();
  }

 private:
  enum Phase { FLYING, HIT, ESCAPED };
  Phase phase = FLYING;
  bool hard = false, shotShown = false;
  float aimX = 64, aimY = 32, duckX = 16, duckY = 32, vx = 0, vy = 0;
  float shotX = 0, shotY = 0;
  int ammo = 3;
  uint32_t completed = 0, lastFrame = 0, spawnedAt = 0, lastTurn = 0, resultAt = 0, shotAt = 0;

  static float aimMotion(int stick) {
    if (abs(stick) <= 650) return 0;
    float speed = 0.65f + min(abs(stick) - 650, 1400) * (1.55f / 1400);
    return stick > 0 ? speed : -speed;
  }
  uint32_t timeLimit() const { return hard ? 4500u : 6500u; }
  void spawn() {
    phase = FLYING; ammo = 3; shotShown = false;
    bool fromLeft = random(2) == 0;
    duckX = fromLeft ? 12 : 115;
    duckY = random(22, 43);
    float speed = (hard ? 1.0f : 0.65f) + min(round - 1, uint32_t(10)) * 0.04f;
    vx = fromLeft ? speed : -speed;
    vy = (random(2) ? 1 : -1) * speed * 0.65f;
    spawnedAt = lastTurn = millis();
  }
  void finish(bool hit) {
    phase = hit ? HIT : ESCAPED; resultAt = millis();
    if (!hit) --lives;
  }
};
