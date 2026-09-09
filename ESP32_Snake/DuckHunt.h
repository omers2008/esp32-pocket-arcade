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
      for (int i = 0; i < birdCount; ++i) {
        Bird &b = birds[i];
        if (b.alive && abs(aimX - b.x) <= 6 + padding && abs(aimY - b.y) <= 4 + padding) {
          b.alive = false; b.hitAt = now; --birdsLeft;
          score += 100 + ammo * 25;
        }
      }
      if (birdsLeft == 0) { finish(true); return; }
      if (ammo == 0) { finish(false); return; }
    }
    if (now - lastFrame < 20) return;
    lastFrame = now;
    aimX = constrain(aimX + aimMotion(stickX), 3.0f, 124.0f);
    aimY = constrain(aimY - aimMotion(stickY), 14.0f, 49.0f);
    bool turn = now - lastTurn >= (hard ? 700u : 1200u);
    if (turn) lastTurn = now;
    for (int i = 0; i < birdCount; ++i) {
      Bird &b = birds[i]; if (!b.alive) continue;
      b.x += b.vx; b.y += b.vy;
      if (b.x < 8) { b.x = 16 - b.x; b.vx = abs(b.vx); }
      if (b.x > 119) { b.x = 238 - b.x; b.vx = -abs(b.vx); }
      if (b.y < 17) { b.y = 34 - b.y; b.vy = abs(b.vy); }
      if (b.y > 46) { b.y = 92 - b.y; b.vy = -abs(b.vy); }
      if (turn && random(3) == 0) b.vy = -b.vy;
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
    for (int i = 0; i < birdCount; ++i) {
      const Bird &b = birds[i];
      if ((b.alive && phase == ESCAPED) || (!b.alive && now - b.hitAt >= 650)) continue;
      int dx = int(b.x), dy = int(b.y), facing = b.vx > 0 ? 1 : -1;
      if (!b.alive) dy = min(49, dy + int((now - b.hitAt) / 35));
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
    d.setCursor(0, 56); d.print(F("A")); d.print(ammo);
    d.setCursor(24, 56); d.print(F("B")); d.print(birdsLeft); d.print('/'); d.print(birdCount);
    d.setCursor(62, 56); d.print(hard ? F("H") : F("E"));
    d.setCursor(80, 56); d.print(F("13:SHOT"));
    d.display();
  }

 private:
  enum Phase { FLYING, HIT, ESCAPED };
  Phase phase = FLYING;
  bool hard = false, shotShown = false;
  struct Bird { float x, y, vx, vy; bool alive; uint32_t hitAt; };
  Bird birds[3] = {};
  float aimX = 64, aimY = 32;
  float shotX = 0, shotY = 0;
  int ammo = 3, birdCount = 1, birdsLeft = 1;
  uint32_t completed = 0, lastFrame = 0, spawnedAt = 0, lastTurn = 0, resultAt = 0, shotAt = 0;

  static float aimMotion(int stick) {
    if (abs(stick) <= 650) return 0;
    float speed = 0.65f + min(abs(stick) - 650, 1400) * (1.55f / 1400);
    return stick > 0 ? speed : -speed;
  }
  uint32_t timeLimit() const { return (hard ? 4500u : 6500u) + (birdCount - 1) * 1500u; }
  void spawn() {
    phase = FLYING; ammo = 3; shotShown = false;
    int roll = random(100);
    birdCount = roll < (hard ? 45 : 60) ? 1 : roll < (hard ? 80 : 90) ? 2 : 3;
    birdsLeft = birdCount;
    bool firstFromLeft = random(2) == 0;
    float speed = (hard ? 1.0f : 0.65f) + min(round - 1, uint32_t(10)) * 0.04f;
    for (int i = 0; i < 3; ++i) {
      bool fromLeft = i % 2 == 0 ? firstFromLeft : !firstFromLeft;
      // Separate launch heights and alternate sides so flocks start spread out.
      birds[i] = {fromLeft ? 12.0f : 115.0f,
                  birdCount == 1 ? float(random(22, 43)) : float(20 + i * 11),
                  fromLeft ? speed : -speed,
                  (random(2) ? 1 : -1) * speed * 0.65f, i < birdCount, 0};
    }
    spawnedAt = lastTurn = millis();
  }
  void finish(bool hit) {
    phase = hit ? HIT : ESCAPED; resultAt = millis();
    if (!hit) --lives;
  }
};
