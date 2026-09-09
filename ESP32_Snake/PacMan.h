#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Original compact maze and monochrome sprites, built for a 128x64 OLED.
class PacMan {
 public:
  uint32_t score = 0, level = 1;
  int lives = 3;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; level = 1; lives = 3; over = false;
    newMaze();
  }

  void update(int stickX, int stickY) {
    if (over) return;
    uint32_t now = millis();
    if (abs(stickX) > 650 || abs(stickY) > 650) {
      wanted = abs(stickX) > abs(stickY) ? (stickX > 0 ? 1 : 3) : (stickY > 0 ? 0 : 2);
    }
    if (now - readyAt < 750) return;
    if (powered && now - powerAt >= powerDuration()) powered = false;
    if (now - lastPlayer >= (hard ? 125u : 140u)) {
      lastPlayer = now;
      if (canMove(px, py, wanted)) direction = wanted;
      if (canMove(px, py, direction)) { px += dx(direction); py += dy(direction); }
      eatPellet();
      if (collide()) return;
      if (remaining == 0) { score += 500; ++level; newMaze(); return; }
    }
    uint32_t ghostMs = powered ? 300u : uint32_t(hard ? max(110, 175 - int(min(level - 1, uint32_t(10))) * 5) :
                                                                  max(165, 230 - int(min(level - 1, uint32_t(10))) * 5));
    if (now - lastGhost < ghostMs) return;
    lastGhost = now;
    int distance[H][W];
    distances(distance);
    for (int i = 0; i < ghostCount(); ++i) {
      Ghost &g = ghosts[i];
      if (!g.active) {
        if (now - g.eatenAt < 2200) continue;
        g.x = homeX(i); g.y = homeY(i); g.dir = -1; g.active = true;
      } else {
        int options[4], count = 0;
        for (int d = 0; d < 4; ++d) {
          if (canMove(g.x, g.y, d) && (g.dir < 0 || d != (g.dir + 2) % 4)) options[count++] = d;
        }
        if (!count && g.dir >= 0 && canMove(g.x, g.y, (g.dir + 2) % 4)) options[count++] = (g.dir + 2) % 4;
        if (count) {
          int best = options[random(count)]; // Random tie-breaking avoids fixed loops.
          if (powered || random(100) >= (hard ? 10 : 35)) {
            for (int n = 0; n < count; ++n) {
              int d = options[n];
              int candidate = distance[g.y + dy(d)][g.x + dx(d)];
              int current = distance[g.y + dy(best)][g.x + dx(best)];
              if (powered ? candidate > current : candidate < current) best = d;
            }
          }
          g.dir = best; g.x += dx(best); g.y += dy(best);
        }
      }
      if (collide()) return;
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    uint32_t now = millis();
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    d.setCursor(0, 0); d.print(score);
    d.setCursor(61, 0); d.print(F("W")); d.print(level);
    d.setCursor(97, 0); d.print(F("L")); d.print(lives);
    d.setCursor(121, 0); d.print(hard ? F("H") : F("E"));
    if (powered) d.drawFastHLine(0, 8, int((powerDuration() - min(now - powerAt, powerDuration())) * 128 / powerDuration()), SSD1306_WHITE);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
      int sx = 11 + x * 5, sy = 9 + y * 5;
      if (!walkable(x, y)) d.fillRect(sx, sy, 5, 5, SSD1306_WHITE);
      else if (pellets[y][x] == 1) d.drawPixel(sx + 2, sy + 2, SSD1306_WHITE);
      else if (pellets[y][x] == 2) d.drawRect(sx + 1, sy + 1, 3, 3, SSD1306_WHITE);
    }
    for (int i = 0; i < ghostCount(); ++i) {
      Ghost &g = ghosts[i]; if (!g.active) continue;
      int sx = 11 + g.x * 5, sy = 9 + g.y * 5;
      if (powered && (now - powerAt < powerDuration() - 1000 || (now / 120) % 2 == 0)) {
        d.drawRect(sx, sy, 5, 5, SSD1306_WHITE);
      } else {
        d.fillRect(sx, sy + 1, 5, 3, SSD1306_WHITE);
        d.drawFastHLine(sx + 1, sy, 3, SSD1306_WHITE);
        d.drawPixel(sx + 1, sy + 2, SSD1306_BLACK); d.drawPixel(sx + 3, sy + 2, SSD1306_BLACK);
        d.drawPixel(sx, sy + 4, SSD1306_WHITE); d.drawPixel(sx + 4, sy + 4, SSD1306_WHITE);
      }
    }
    if (now - readyAt >= 1800 || (now / 100) % 2 == 0) {
      int sx = 11 + px * 5, sy = 9 + py * 5;
      d.fillRect(sx + 1, sy, 3, 5, SSD1306_WHITE);
      d.fillRect(sx, sy + 1, 5, 3, SSD1306_WHITE);
      if ((now / 100) % 2 == 0 && direction >= 0)
        d.drawLine(sx + 2, sy + 2, sx + 2 + 2 * dx(direction), sy + 2 + 2 * dy(direction), SSD1306_BLACK);
    }
    if (now - readyAt < 750) {
      d.fillRect(40, 27, 48, 12, SSD1306_BLACK);
      d.setCursor(46, 29); d.print(F("READY!"));
    }
    d.display();
  }

 private:
  static constexpr int W = 21, H = 11;
  struct Ghost { int x, y, dir; bool active; uint32_t eatenAt; };
  Ghost ghosts[3];
  uint8_t pellets[H][W] = {};
  int px = 10, py = 9, direction = -1, wanted = -1, remaining = 0, combo = 0;
  bool hard = false, powered = false;
  uint32_t readyAt = 0, lastPlayer = 0, lastGhost = 0, powerAt = 0;

  static bool walkable(int x, int y) {
    static const char maze[H][W + 1] = {
      "#####################",
      "#.....#.......#.....#",
      "#.###.#.#####.#.###.#",
      "#...................#",
      "#.###.###.#.###.###.#",
      "#.....#...#...#.....#",
      "#.###.#.#.#.#.#.###.#",
      "#...#...#...#...#...#",
      "###.#.###.#.###.#.###",
      "#...................#",
      "#####################"
    };
    return x >= 0 && x < W && y >= 0 && y < H && maze[y][x] == '.';
  }
  static int dx(int dir) { return dir == 1 ? 1 : dir == 3 ? -1 : 0; }
  static int dy(int dir) { return dir == 2 ? 1 : dir == 0 ? -1 : 0; }
  static bool canMove(int x, int y, int dir) { return dir >= 0 && dir < 4 && walkable(x + dx(dir), y + dy(dir)); }
  int ghostCount() const { return hard ? 3 : 2; }
  uint32_t powerDuration() const { return hard ? 3500u : 6000u; }
  static int homeX(int i) { return i == 1 ? 1 : 19; }
  static int homeY(int i) { return i == 2 ? 9 : 1; }

  void resetActors() {
    px = 10; py = 9; direction = wanted = -1; powered = false; combo = 0;
    readyAt = lastPlayer = lastGhost = millis();
    for (int i = 0; i < 3; ++i) ghosts[i] = {homeX(i), homeY(i), -1, true, 0};
  }
  void newMaze() {
    remaining = 0;
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
      pellets[y][x] = walkable(x, y) ? 1 : 0;
      if ((x == 1 || x == 19) && (y == 1 || y == 9)) pellets[y][x] = 2;
      if (x == 10 && y == 9) pellets[y][x] = 0;
      if (pellets[y][x]) ++remaining;
    }
    resetActors();
  }
  void eatPellet() {
    uint8_t &p = pellets[py][px];
    if (!p) return;
    score += p == 2 ? 50 : 10;
    if (p == 2) {
      powered = true; powerAt = millis(); combo = 0;
      for (auto &g : ghosts) if (g.dir >= 0) g.dir = (g.dir + 2) % 4;
    }
    p = 0; --remaining;
  }
  bool collide() {
    for (int i = 0; i < ghostCount(); ++i) {
      Ghost &g = ghosts[i];
      if (!g.active || g.x != px || g.y != py) continue;
      if (powered) {
        score += uint32_t(200) << min(combo, 3); ++combo;
        g.active = false; g.eatenAt = millis();
      } else if (millis() - readyAt >= 1800) {
        if (--lives == 0) over = true;
        else resetActors();
        return true;
      }
    }
    return false;
  }
  void distances(int (&dist)[H][W]) const {
    uint16_t queue[W * H]; int head = 0, tail = 0;
    for (auto &row : dist) for (auto &cell : row) cell = W * H;
    dist[py][px] = 0; queue[tail++] = py * W + px;
    while (head < tail) {
      int cell = queue[head++], x = cell % W, y = cell / W;
      for (int d = 0; d < 4; ++d) {
        int nx = x + dx(d), ny = y + dy(d);
        if (!walkable(nx, ny) || dist[ny][nx] != W * H) continue;
        dist[ny][nx] = dist[y][x] + 1; queue[tail++] = ny * W + nx;
      }
    }
  }
};
