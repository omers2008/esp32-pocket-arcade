#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Original seven-column, six-row four-in-a-row game against the CPU.
class FourInRow {
 public:
  uint32_t score = 0;
  bool over = false, won = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; over = won = false;
    for (int y = 0; y < ROWS; ++y) for (int x = 0; x < COLS; ++x) board[y][x] = 0;
    selectedCol = 3;
    stickReady = false;
  }

  void update(int stickX, bool dropPressed) {
    if (over) return;
    if (abs(stickX) < 350) stickReady = true;
    if (stickReady && abs(stickX) > 650) {
      selectedCol = (selectedCol + COLS + (stickX > 0 ? 1 : -1)) % COLS;
      stickReady = false;
    }
    if (dropPressed) dropPlayer();
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    d.setCursor(0, 0); d.print(F("4 IN ROW"));
    d.setCursor(52, 0); d.print(hard ? 'H' : 'E');
    d.setCursor(68, 0); d.print(F("13"));
    const int x0 = 70, y0 = 14, cell = 8;
    d.drawRect(x0 - 1, y0 - 1, COLS * cell + 2, ROWS * cell + 2, SSD1306_WHITE);
    int markerX = x0 + selectedCol * cell + 3;
    d.drawFastVLine(markerX, 10, 3, SSD1306_WHITE);
    for (int y = 0; y < ROWS; ++y) for (int x = 0; x < COLS; ++x) {
      int cx = x0 + x * cell + 4, cy = y0 + y * cell + 4;
      if (board[y][x] == 1) d.fillCircle(cx, cy, 3, SSD1306_WHITE);
      else if (board[y][x] == 2) d.drawCircle(cx, cy, 3, SSD1306_WHITE);
    }
    d.setCursor(0, 22); d.print(F("FILLED=YOU"));
    d.setCursor(0, 34); d.print(F("RING=CPU"));
    d.setCursor(0, 49); d.print(F("LEFT/RIGHT"));
    d.setCursor(0, 59); d.print(F("13 DROP"));
    d.display();
  }

 private:
  static constexpr int COLS = 7, ROWS = 6;
  uint8_t board[ROWS][COLS] = {};
  int selectedCol = 3;
  bool hard = false, stickReady = false;

  int landingRow(int col) const {
    for (int y = ROWS - 1; y >= 0; --y) if (!board[y][col]) return y;
    return -1;
  }

  bool hasFour(int player) const {
    static const int dx[] = {1, 0, 1, 1};
    static const int dy[] = {0, 1, 1, -1};
    for (int y = 0; y < ROWS; ++y) for (int x = 0; x < COLS; ++x) {
      if (board[y][x] != player) continue;
      for (int dir = 0; dir < 4; ++dir) {
        int count = 1;
        for (int n = 1; n < 4; ++n) {
          int nx = x + dx[dir] * n, ny = y + dy[dir] * n;
          if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS || board[ny][nx] != player) break;
          ++count;
        }
        if (count == 4) return true;
      }
    }
    return false;
  }

  bool full() const {
    for (int x = 0; x < COLS; ++x) if (!board[0][x]) return false;
    return true;
  }

  bool wouldWin(int col, int player) {
    int row = landingRow(col); if (row < 0) return false;
    board[row][col] = player;
    bool result = hasFour(player);
    board[row][col] = 0;
    return result;
  }

  int cpuColumn() {
    if (hard) {
      for (int col = 0; col < COLS; ++col) if (wouldWin(col, 2)) return col;
      for (int col = 0; col < COLS; ++col) if (wouldWin(col, 1)) return col;
      if (landingRow(3) >= 0) return 3;
    }
    int valid[COLS], count = 0;
    for (int col = 0; col < COLS; ++col) if (landingRow(col) >= 0) valid[count++] = col;
    return count ? valid[random(count)] : -1;
  }

  void drop(int col, int player) {
    int row = landingRow(col); if (row < 0) return;
    board[row][col] = player;
  }

  void dropPlayer() {
    if (landingRow(selectedCol) < 0) return;
    drop(selectedCol, 1);
    if (hasFour(1)) { score = 1000; won = over = true; return; }
    if (full()) { score = 100; over = true; return; }
    int col = cpuColumn();
    if (col < 0) { score = 100; over = true; return; }
    drop(col, 2);
    if (hasFour(2)) { score = 0; over = true; return; }
    if (full()) { score = 100; over = true; }
  }
};
