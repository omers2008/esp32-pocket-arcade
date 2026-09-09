#pragma once

// A complete, small-screen Space Invaders game. Coordinates are OLED pixels.
class Invaders {
 public:
  uint32_t score = 0;
  uint16_t wave = 1;
  uint8_t lives = 3;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode;
    score = 0;
    wave = 1;
    lives = hard ? 2 : 3;
    over = false;
    playerX = 60;
    immuneUntil = 0;
    lastFrame = millis();
    lastShot = lastFrame - 250;
    newWave();
  }

  void update(int stickX, bool fire) {
    uint32_t now = millis();
    if (over || now - lastFrame < 20) return;
    lastFrame = now;
    if (stickX > 650) playerX = min(playerX + 2, 117);
    if (stickX < -650) playerX = max(playerX - 2, 1);

    if (fire && now - lastShot >= 220) {
      for (auto &b : shots) {
        if (!b.active) {
          b = {playerX + 4, 55, true};
          lastShot = now;
          break;
        }
      }
    }
    for (auto &b : shots) {
      if (!b.active) continue;
      b.y -= 3;
      if (b.y < 9) { b.active = false; continue; }
      for (int r = 0; r < 3 && b.active; ++r) {
        for (int c = 0; c < 6 && b.active; ++c) {
          int ax = originX + c * 16;
          int ay = originY + r * 10;
          if (alive[r][c] && b.x >= ax && b.x < ax + 10 &&
              b.y <= ay + 6 && b.y + 3 >= ay) {
            alive[r][c] = false;
            b.active = false;
            score += (3 - r) * 10;
            --remaining;
          }
        }
      }
    }
    if (remaining == 0) { ++wave; newWave(); return; }

    int stepMs = max(hard ? 65 : 90, (hard ? 340 : 480) - int(wave - 1) * 35 - (18 - remaining) * 15);
    if (now - lastStep >= uint32_t(stepMs)) {
      lastStep = now;
      bool edge = false;
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 6; ++c) {
          if (!alive[r][c]) continue;
          int nextX = originX + c * 16 + march * 3;
          if (nextX < 1 || nextX + 10 > 127) edge = true;
        }
      }
      if (edge) { march = -march; originY += 3; }
      else originX += march * 3;
      animation = !animation;
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 6; ++c) {
          if (alive[r][c] && originY + r * 10 + 7 >= 55) over = true;
        }
      }
      if (over) return;
    }

    int fireMs = max(hard ? 200 : 300, (hard ? 550 : 950) - int(wave - 1) * 70);
    if (now - lastEnemyShot >= uint32_t(fireMs)) {
      lastEnemyShot = now;
      int column = random(6);
      for (int offset = 0; offset < 6; ++offset) {
        int c = (column + offset) % 6;
        bool found = false;
        for (int r = 2; r >= 0; --r) {
          if (!alive[r][c]) continue;
          for (auto &b : bombs) {
            if (!b.active) { b = {originX + c * 16 + 5, originY + r * 10 + 7, true}; break; }
          }
          found = true;
          break;
        }
        if (found) break;
      }
    }
    for (auto &b : bombs) {
      if (!b.active) continue;
      b.y += hard ? 2 : 1;
      if (b.y > 63) { b.active = false; continue; }
      if (b.x >= playerX && b.x < playerX + 9 && b.y + 3 >= 56 && b.y <= 61) {
        b.active = false;
        if (int32_t(now - immuneUntil) >= 0) {
          --lives;
          immuneUntil = now + 1200;
          if (lives == 0) over = true;
        }
      }
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay();
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    d.setCursor(0, 0); d.print(score);
    d.setCursor(55, 0); d.print(F("W")); d.print(wave);
    d.setCursor(85, 0); d.print(hard ? F("H") : F("E"));
    d.setCursor(98, 0); d.print(F("HP")); d.print(lives);
    d.drawFastHLine(0, 8, 128, SSD1306_WHITE);
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 6; ++c) {
        if (!alive[r][c]) continue;
        int x = originX + c * 16, y = originY + r * 10;
        d.fillRect(x + 2, y + 1, 6, 4, SSD1306_WHITE);
        d.drawFastHLine(x, y + 3, 10, SSD1306_WHITE);
        d.drawPixel(x + 3, y + 2, SSD1306_BLACK);
        d.drawPixel(x + 6, y + 2, SSD1306_BLACK);
        d.drawPixel(x + 2, y, SSD1306_WHITE);
        d.drawPixel(x + 7, y, SSD1306_WHITE);
        d.drawPixel(x + (animation ? 1 : 3), y + 5, SSD1306_WHITE);
        d.drawPixel(x + (animation ? 8 : 6), y + 5, SSD1306_WHITE);
      }
    }
    uint32_t now = millis();
    if (int32_t(now - immuneUntil) >= 0 || (now / 100) % 2 == 0) {
      d.fillRect(playerX, 59, 9, 3, SSD1306_WHITE);
      d.fillRect(playerX + 2, 57, 5, 3, SSD1306_WHITE);
      d.drawFastVLine(playerX + 4, 55, 3, SSD1306_WHITE);
    }
    for (auto &b : shots) if (b.active) d.drawFastVLine(b.x, b.y, 3, SSD1306_WHITE);
    for (auto &b : bombs) if (b.active) d.drawFastVLine(b.x, b.y, 3, SSD1306_WHITE);
    d.display();
  }

 private:
  struct Bullet { int x = 0; int y = 0; bool active = false; };
  Bullet shots[3], bombs[3];
  bool alive[3][6];
  int originX = 18, originY = 12, march = 1, remaining = 18, playerX = 60;
  bool animation = false;
  bool hard = false;
  uint32_t lastFrame = 0, lastStep = 0, lastShot = 0, lastEnemyShot = 0, immuneUntil = 0;

  void newWave() {
    originX = 18; originY = 12; march = 1; remaining = 18;
    for (auto &row : alive) for (auto &a : row) a = true;
    for (auto &b : shots) b.active = false;
    for (auto &b : bombs) b.active = false;
    lastStep = lastEnemyShot = millis();
  }
};
