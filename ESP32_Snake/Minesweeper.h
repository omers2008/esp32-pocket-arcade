#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Original small-screen Minesweeper. The first dig is always safe.
class Minesweeper {
 public:
  uint32_t score = 0;
  bool over = false, won = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; over = won = false;
    width = hard ? 12 : 8;
    height = 6;
    mineTotal = hard ? 16 : 8;
    cursorX = width / 2; cursorY = height / 2;
    stickReady = false; firstDig = true; flags = 0;
    for (int y = 0; y < MAX_H; ++y) for (int x = 0; x < MAX_W; ++x) {
      mine[y][x] = false; revealed[y][x] = false; flagged[y][x] = false; adjacent[y][x] = 0;
    }
  }

  void update(int stickX, int stickY, bool digPressed, bool flagPressed) {
    if (over) return;
    if (abs(stickX) < 350 && abs(stickY) < 350) stickReady = true;
    if (stickReady && (abs(stickX) > 650 || abs(stickY) > 650)) {
      if (abs(stickX) >= abs(stickY)) cursorX = constrain(cursorX + (stickX > 0 ? 1 : -1), 0, width - 1);
      else cursorY = constrain(cursorY + (stickY > 0 ? -1 : 1), 0, height - 1);
      stickReady = false;
    }
    if (digPressed) dig();
    else if (flagPressed) toggleFlag();
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    d.setCursor(0, 0); d.print(F("MINES ")); d.print(hard ? 'H' : 'E');
    d.setCursor(51, 0); d.print(F("M:")); d.print(mineTotal - flags);
    d.setCursor(91, 0); d.print(F("13D 14F"));
    int cell = 8, x0 = (128 - width * cell) / 2, y0 = 12;
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
      int px = x0 + x * cell, py = y0 + y * cell;
      if (!revealed[y][x]) {
        d.drawRect(px + 1, py + 1, 6, 6, SSD1306_WHITE);
        if (flagged[y][x]) {
          d.drawLine(px + 2, py + 5, px + 5, py + 2, SSD1306_WHITE);
          d.drawFastVLine(px + 2, py + 2, 5, SSD1306_WHITE);
        }
      } else if (mine[y][x]) {
        d.fillCircle(px + 4, py + 4, 3, SSD1306_WHITE);
      } else if (adjacent[y][x]) {
        d.setCursor(px + 2, py + 1); d.print(adjacent[y][x]);
      }
    }
    int cx = x0 + cursorX * cell, cy = y0 + cursorY * cell;
    d.drawRect(cx, cy, cell, cell, SSD1306_WHITE);
    d.display();
  }

 private:
  static constexpr int MAX_W = 12, MAX_H = 6;
  int width = 8, height = 6, mineTotal = 8, flags = 0;
  int cursorX = 4, cursorY = 3;
  bool hard = false, firstDig = true, stickReady = false;
  bool mine[MAX_H][MAX_W] = {};
  bool revealed[MAX_H][MAX_W] = {};
  bool flagged[MAX_H][MAX_W] = {};
  uint8_t adjacent[MAX_H][MAX_W] = {};

  bool inBounds(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height; }

  void placeMines() {
    int placed = 0;
    while (placed < mineTotal) {
      int x = random(width), y = random(height);
      if ((x == cursorX && y == cursorY) || mine[y][x]) continue;
      mine[y][x] = true; ++placed;
    }
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
      int count = 0;
      for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx)
        if ((dx || dy) && inBounds(x + dx, y + dy) && mine[y + dy][x + dx]) ++count;
      adjacent[y][x] = count;
    }
    firstDig = false;
  }

  void revealAllMines() {
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
      if (mine[y][x]) revealed[y][x] = true;
  }

  void floodReveal(int startX, int startY) {
    int stackX[MAX_W * MAX_H], stackY[MAX_W * MAX_H], top = 0;
    stackX[top] = startX; stackY[top++] = startY;
    while (top) {
      int x = stackX[--top], y = stackY[top];
      if (!inBounds(x, y) || revealed[y][x] || flagged[y][x] || mine[y][x]) continue;
      revealed[y][x] = true;
      if (adjacent[y][x]) continue;
      for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx)
        if ((dx || dy) && inBounds(x + dx, y + dy) && top < MAX_W * MAX_H) {
          stackX[top] = x + dx; stackY[top++] = y + dy;
        }
    }
  }

  bool cleared() const {
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
      if (!mine[y][x] && !revealed[y][x]) return false;
    return true;
  }

  void dig() {
    if (flagged[cursorY][cursorX]) return;
    if (firstDig) placeMines();
    if (mine[cursorY][cursorX]) {
      revealAllMines(); score = 0; over = true; return;
    }
    floodReveal(cursorX, cursorY);
    if (cleared()) { score = hard ? 1500 : 1000; won = over = true; }
  }

  void toggleFlag() {
    if (revealed[cursorY][cursorX]) return;
    if (flagged[cursorY][cursorX]) { flagged[cursorY][cursorX] = false; --flags; }
    else if (flags < mineTotal) { flagged[cursorY][cursorX] = true; ++flags; }
  }
};
