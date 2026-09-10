#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// A small original top-down adventure designed for the 128x64 OLED.
// The player starts with only a directional punch. The first reward fight
// grants a sword and the second grants a bow; rocks and arrows are separate
// physical pickups so the controls stay readable on three buttons.
class TopdownRPG {
 public:
  uint32_t score = 0;
  bool over = false, won = false;
  bool swordUnlocked = false, bowUnlocked = false, holdingRock = false;
  int hp = 5, ammo = 0;

  void start(bool hardMode) {
    hard = hardMode;
    score = 0;
    hp = hard ? 3 : 5;
    ammo = 0;
    over = won = false;
    started = false;
    swordUnlocked = bowUnlocked = holdingRock = false;
    x = 22; y = 52; facingX = 1; facingY = 0;
    attackTicks = attackCooldown = immune = 0;
    itemQueued = false;
    message = 0;
    for (auto &r : rocks) r.available = true;
    rocks[0] = {40, 31, true, 0};
    rocks[1] = {86, 72, true, 0};
    rocks[2] = {145, 27, true, 0};
    rocks[3] = {188, 87, true, 0};
    rocks[4] = {235, 45, true, 0};
    for (int i = 0; i < ROCK_COUNT; ++i) rocks[i].respawn = 0;

    enemies[0] = {58, 31, hard ? 3 : 2, hard ? 3 : 2, REWARD_SWORD, true, false};
    enemies[1] = {96, 74, hard ? 3 : 2, hard ? 3 : 2, REWARD_NONE, true, false};
    enemies[2] = {144, 25, hard ? 4 : 3, hard ? 4 : 3, REWARD_BOW, true, false};
    enemies[3] = {179, 82, hard ? 3 : 2, hard ? 3 : 2, REWARD_NONE, true, false};
    enemies[4] = {224, 32, hard ? 4 : 3, hard ? 4 : 3, REWARD_NONE, true, false};
    enemies[5] = {121, 96, hard ? 3 : 2, hard ? 3 : 2, REWARD_NONE, true, false};
    for (auto &a : arrows) a = {0, 0, 0, 0, false, false};
    rockShot = {0, 0, 0, 0, -1, false};
    lastFrame = millis();
  }

  // stickX/stickY are centered joystick readings. actionHeld is GPIO13;
  // itemPressed is the debounced one-shot event from GPIO14.
  void update(int stickX, int stickY, bool actionHeld, bool itemPressed) {
    if (over) return;
    itemQueued |= itemPressed;
    uint32_t now = millis();
    if (!started) {
      if (actionHeld) { started = true; lastFrame = now; }
      itemQueued = false;
      return;
    }
    if (now - lastFrame < 20) return;
    lastFrame = now;

    if (attackCooldown > 0) --attackCooldown;
    if (attackTicks > 0) --attackTicks;
    if (immune > 0) --immune;
    for (auto &r : rocks) {
      if (r.respawn > 0 && --r.respawn == 0) r.available = true;
    }

    int moveX = stickX > 650 ? 1 : stickX < -650 ? -1 : 0;
    int moveY = stickY > 650 ? -1 : stickY < -650 ? 1 : 0;
    if (moveX) facingX = moveX, facingY = 0;
    else if (moveY) facingX = 0, facingY = moveY;
    x = constrain(x + moveX * MOVE_SPEED, 5.0f, WORLD_WIDTH - 12.0f);
    y = constrain(y + moveY * MOVE_SPEED, 12.0f, WORLD_HEIGHT - 10.0f);

    if (itemQueued) useItem();
    itemQueued = false;

    if (actionHeld && attackCooldown == 0) {
      attackTicks = 6;
      attackCooldown = 12;
      attackHit = false;
    }
    if (attackTicks > 0 && !attackHit) {
      float ax = x + (facingX > 0 ? 7 : facingX < 0 ? -19 : 1);
      float ay = y + (facingY > 0 ? 7 : facingY < 0 ? -12 : 1);
      float aw = facingX ? 19 : 5;
      float ah = facingY ? 19 : 5;
      for (auto &e : enemies) {
        if (!e.alive || !overlaps(ax, ay, aw, ah, e.x, e.y, 8, 8)) continue;
        e.hit = true;
        --e.hp;
        attackHit = true;
        if (e.hp <= 0) defeat(e);
        break;
      }
    }

    updateRockShot();
    updateArrows();
    updateEnemies();
    if (allEnemiesDefeated()) { won = over = true; }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay();
    d.setTextColor(SSD1306_WHITE);
    d.setTextSize(1);
    if (!started) {
      line(d, 0, "FOREST QUEST");
      line(d, 12, "L/R/U/D explore");
      line(d, 23, "13 punch / sword");
      line(d, 34, "14 pick + throw rock");
      line(d, 45, "Fight for sword + bow");
      line(d, 56, "13: enter forest");
      d.display();
      return;
    }

    int cameraX = constrain(int(x) - 58, 0, WORLD_WIDTH - 128);
    int cameraY = constrain(int(y) - 31, 0, WORLD_HEIGHT - PLAY_HEIGHT);
    for (int wx = cameraX; wx < cameraX + 128; wx += 8) {
      for (int wy = cameraY; wy < cameraY + PLAY_HEIGHT; wy += 8) {
        if (((wx / 8) + (wy / 8)) % 2 == 0) d.drawPixel(wx - cameraX + 2, wy - cameraY + 11, SSD1306_WHITE);
      }
    }
    drawTrees(d, cameraX, cameraY);
    for (auto &r : rocks) if (r.available) drawRock(d, int(r.x) - cameraX, int(r.y) - cameraY + 10);
    for (auto &a : arrows) if (a.recoverable) drawArrow(d, int(a.x) - cameraX, int(a.y) - cameraY + 10, 0, 0);
    if (rockShot.active) drawRock(d, int(rockShot.x) - cameraX, int(rockShot.y) - cameraY + 10);
    for (auto &a : arrows) if (a.active) drawArrow(d, int(a.x) - cameraX, int(a.y) - cameraY + 10, a.vx, a.vy);
    for (auto &e : enemies) if (e.alive) {
      int ex = int(e.x) - cameraX, ey = int(e.y) - cameraY + 10;
      d.fillRect(ex + 1, ey + 1, 6, 6, SSD1306_WHITE);
      d.drawPixel(ex + 2, ey + 2, SSD1306_BLACK);
      d.drawPixel(ex + 5, ey + 2, SSD1306_BLACK);
      if (e.reward == REWARD_SWORD && !swordUnlocked) { d.setCursor(ex - 1, ey - 9); d.print('S'); }
      if (e.reward == REWARD_BOW && !bowUnlocked) { d.setCursor(ex - 1, ey - 9); d.print('B'); }
    }
    if (rockShot.active) drawRock(d, int(rockShot.x) - cameraX, int(rockShot.y) - cameraY + 10);

    if (!immune || immune % 8 < 4) {
      int px = int(x) - cameraX, py = int(y) - cameraY + 10;
      d.fillRect(px + 1, py + 1, 5, 5, SSD1306_WHITE);
      d.drawPixel(px + (facingX > 0 ? 6 : facingX < 0 ? 0 : 3), py + 3, SSD1306_WHITE);
      d.drawPixel(px + 2, py + 2, SSD1306_BLACK);
      d.drawPixel(px + 4, py + 2, SSD1306_BLACK);
    }
    if (attackTicks) {
      int sx = int(x) - cameraX, sy = int(y) - cameraY + 10;
      if (swordUnlocked) {
        if (facingX) d.drawLine(sx + (facingX > 0 ? 7 : -1), sy + 2, sx + (facingX > 0 ? 20 : -14), sy + 8, SSD1306_WHITE);
        else d.drawLine(sx + 2, sy + (facingY > 0 ? 7 : -1), sx + 8, sy + (facingY > 0 ? 20 : -14), SSD1306_WHITE);
      } else {
        if (facingX) d.drawFastHLine(sx + (facingX > 0 ? 7 : -12), sy + 4, 12, SSD1306_WHITE);
        else d.drawFastVLine(sx + 4, sy + (facingY > 0 ? 7 : -12), 12, SSD1306_WHITE);
      }
    }
    d.fillRect(0, 0, 128, 9, SSD1306_BLACK);
    d.setCursor(0, 0); d.print(F("HP")); d.print(hp);
    d.setCursor(24, 0); d.print(score);
    d.setCursor(57, 0);
    if (bowUnlocked) { d.print(F("B")); d.print(ammo); }
    else if (swordUnlocked) d.print(F("SWORD"));
    else if (holdingRock) d.print(F("ROCK"));
    else d.print(F("PUNCH"));
    d.setCursor(116, 0); d.print(hard ? 'H' : 'E');
    d.drawFastHLine(0, 9, 128, SSD1306_WHITE);
    if (message && millis() - messageAt < 1000) {
      d.fillRect(6, 12, 116, 10, SSD1306_BLACK);
      d.setCursor(8, 13);
      d.print(message == 1 ? F("SWORD UNLOCKED") : message == 2 ? F("BOW! 5 ARROWS") :
              message == 3 ? F("ROCK PICKED") : message == 4 ? F("ARROW CRAFTED") :
              message == 5 ? F("ARROW RECOVERED") : F("NO AMMO / FIND ROCK"));
    }
    d.display();
  }

 private:
  static constexpr int WORLD_WIDTH = 256;
  static constexpr int WORLD_HEIGHT = 110;
  static constexpr int PLAY_HEIGHT = 54;
  static constexpr int ROCK_COUNT = 5;
  static constexpr int ENEMY_COUNT = 6;
  static constexpr int ARROW_COUNT = 5;
  static constexpr float MOVE_SPEED = 1.6f;
  enum Reward : uint8_t { REWARD_NONE, REWARD_SWORD, REWARD_BOW };
  struct Rock { float x, y; bool available; int respawn; };
  struct Enemy { float x, y; int hp, maxHp; Reward reward; bool alive, hit; };
  struct Arrow { float x, y, vx, vy; bool active, recoverable; };
  struct RockShot { float x, y, vx, vy; int source; bool active; };

  Rock rocks[ROCK_COUNT];
  Enemy enemies[ENEMY_COUNT];
  Arrow arrows[ARROW_COUNT];
  RockShot rockShot;
  bool hard = false, started = false, itemQueued = false, attackHit = false;
  float x = 22, y = 52;
  int facingX = 1, facingY = 0;
  int attackTicks = 0, attackCooldown = 0, immune = 0;
  int message = 0;
  uint32_t lastFrame = 0, messageAt = 0;

  static bool overlaps(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
  }

  bool near(float ax, float ay, float bx, float by, float distance = 10.0f) const {
    return abs(ax - bx) <= distance && abs(ay - by) <= distance;
  }

  int nearbyRock() const {
    for (int i = 0; i < ROCK_COUNT; ++i)
      if (rocks[i].available && near(x + 3, y + 3, rocks[i].x, rocks[i].y)) return i;
    return -1;
  }

  int nearbyArrow() const {
    for (int i = 0; i < ARROW_COUNT; ++i)
      if (arrows[i].recoverable && near(x + 3, y + 3, arrows[i].x, arrows[i].y, 11.0f)) return i;
    return -1;
  }

  void useItem() {
    if (bowUnlocked) {
      int recovered = ammo < 5 ? nearbyArrow() : -1;
      if (recovered >= 0) {
        arrows[recovered].recoverable = false;
        ammo = min(ammo + 1, 5);
        message = 5; messageAt = millis();
      } else if (ammo > 0) {
        fireArrow();
      } else {
        int rock = nearbyRock();
        if (rock >= 0) {
          rocks[rock].available = false;
          rocks[rock].respawn = hard ? 240 : 180;
          ammo = 5;
          message = 4; messageAt = millis();
        } else {
          message = 6; messageAt = millis();
        }
      }
      return;
    }
    if (holdingRock) {
      throwRock();
      return;
    }
    int rock = nearbyRock();
    if (rock >= 0) {
      holdingRock = true;
      rocks[rock].available = false;
      rockShot.source = rock;
      message = 3; messageAt = millis();
    }
  }

  void throwRock() {
    if (rockShot.active) return;
    rockShot = {x + 3, y + 3, float(facingX) * 2.8f, float(facingY) * 2.8f, rockShot.source, true};
    if (rockShot.vx == 0 && rockShot.vy == 0) rockShot.vx = 2.8f;
    holdingRock = false;
  }

  void fireArrow() {
    for (auto &a : arrows) if (!a.active && !a.recoverable) {
      a = {x + 3, y + 3, float(facingX) * 4.0f, float(facingY) * 4.0f, true, false};
      if (a.vx == 0 && a.vy == 0) a.vx = 4.0f;
      --ammo;
      return;
    }
    message = 5; messageAt = millis();
  }

  void updateRockShot() {
    if (!rockShot.active) return;
    rockShot.x += rockShot.vx;
    rockShot.y += rockShot.vy;
    for (auto &e : enemies) {
      if (e.alive && overlaps(rockShot.x - 2, rockShot.y - 2, 5, 5, e.x, e.y, 8, 8)) {
        --e.hp;
        rockShot.active = false;
        restoreThrownRock();
        if (e.hp <= 0) defeat(e);
        return;
      }
    }
    if (rockShot.x < 2 || rockShot.x > WORLD_WIDTH - 2 || rockShot.y < 11 || rockShot.y > WORLD_HEIGHT) {
      rockShot.active = false;
      restoreThrownRock();
    }
  }

  void restoreThrownRock() {
    if (rockShot.source >= 0 && rockShot.source < ROCK_COUNT) rocks[rockShot.source].available = true;
    rockShot.source = -1;
  }

  void updateArrows() {
    for (auto &a : arrows) {
      if (!a.active) continue;
      a.x += a.vx; a.y += a.vy;
      bool hit = false;
      for (auto &e : enemies) {
        if (e.alive && overlaps(a.x - 1, a.y - 1, 4, 4, e.x, e.y, 8, 8)) {
          --e.hp; hit = true;
          if (e.hp <= 0) defeat(e);
          break;
        }
      }
      if (hit) { a.active = false; a.recoverable = true; }
      else if (a.x < 2 || a.x > WORLD_WIDTH - 2 || a.y < 11 || a.y > WORLD_HEIGHT) a.active = false;
    }
  }

  void updateEnemies() {
    for (auto &e : enemies) {
      if (!e.alive) continue;
      float dx = x - e.x, dy = y - e.y;
      if (abs(dx) < 55 && abs(dy) < 45) {
        float speed = hard ? 0.42f : 0.28f;
        if (abs(dx) > 4) e.x += dx > 0 ? speed : -speed;
        if (abs(dy) > 4) e.y += dy > 0 ? speed : -speed;
      }
      if (!immune && overlaps(x, y, 7, 7, e.x, e.y, 8, 8)) {
        --hp;
        immune = hard ? 45 : 65;
        if (hp <= 0) { hp = 0; over = true; return; }
      }
    }
  }

  void defeat(Enemy &e) {
    e.alive = false;
    score += e.reward == REWARD_SWORD ? 150 : e.reward == REWARD_BOW ? 250 : 50;
    if (e.reward == REWARD_SWORD && !swordUnlocked) {
      swordUnlocked = true;
      message = 1; messageAt = millis();
    } else if (e.reward == REWARD_BOW && !bowUnlocked) {
      bowUnlocked = true;
      ammo = 5;
      if (holdingRock) { holdingRock = false; restoreThrownRock(); }
      message = 2; messageAt = millis();
    }
  }

  bool allEnemiesDefeated() const {
    for (const auto &e : enemies) if (e.alive) return false;
    return true;
  }

  static void drawRock(Adafruit_SSD1306 &d, int px, int py) {
    d.drawLine(px + 3, py, px + 7, py + 3, SSD1306_WHITE);
    d.drawLine(px + 7, py + 3, px + 3, py + 7, SSD1306_WHITE);
    d.drawLine(px + 3, py + 7, px, py + 3, SSD1306_WHITE);
    d.drawLine(px, py + 3, px + 3, py, SSD1306_WHITE);
  }

  static void drawArrow(Adafruit_SSD1306 &d, int px, int py, float vx, float vy) {
    if (abs(vx) >= abs(vy)) d.drawFastHLine(px - 3, py, 7, SSD1306_WHITE);
    else d.drawFastVLine(px, py - 3, 7, SSD1306_WHITE);
  }

  static void drawTrees(Adafruit_SSD1306 &d, int cameraX, int cameraY) {
    static const int trees[][2] = {{18, 18}, {76, 22}, {112, 91}, {166, 55}, {207, 18}, {244, 84}};
    for (auto &tree : trees) {
      int tx = tree[0] - cameraX, ty = tree[1] - cameraY + 10;
      if (tx < -8 || tx > 128 || ty < 4 || ty > 64) continue;
      d.fillRect(tx + 3, ty + 5, 3, 7, SSD1306_WHITE);
      d.drawCircle(tx + 4, ty + 3, 5, SSD1306_WHITE);
    }
  }

  static void line(Adafruit_SSD1306 &d, int y, const char *text) {
    d.setCursor(0, y); d.print(text);
  }
};
