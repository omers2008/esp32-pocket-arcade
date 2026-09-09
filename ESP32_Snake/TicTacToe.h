#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Original three-by-three Tic-Tac-Toe against the CPU.
class TicTacToe {
 public:
  uint32_t score = 0;
  bool over = false, won = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; over = won = false;
    for (auto &cell : board) cell = 0;
    cursorX = cursorY = 1;
    stickReady = false;
  }

  void update(int stickX, int stickY, bool placePressed) {
    if (over) return;
    if (abs(stickX) < 350 && abs(stickY) < 350) stickReady = true;
    if (stickReady && (abs(stickX) > 650 || abs(stickY) > 650)) {
      if (abs(stickX) >= abs(stickY)) cursorX = (cursorX + 3 + (stickX > 0 ? 1 : -1)) % 3;
      else cursorY = (cursorY + 3 + (stickY > 0 ? -1 : 1)) % 3;
      stickReady = false;
    }
    if (placePressed && board[cursorY * 3 + cursorX] == 0) placePlayer();
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    d.setCursor(0, 0); d.print(F("TIC TAC TOE  ")); d.print(hard ? 'H' : 'E');
    d.setCursor(84, 0); d.print(F("13 PLACE"));
    const int x0 = 39, y0 = 11, cell = 17;
    for (int i = 1; i < 3; ++i) {
      d.drawFastVLine(x0 + i * cell, y0, 51, SSD1306_WHITE);
      d.drawFastHLine(x0, y0 + i * cell, 51, SSD1306_WHITE);
    }
    d.drawRect(x0 + cursorX * cell + 1, y0 + cursorY * cell + 1, cell - 2, cell - 2, SSD1306_WHITE);
    for (int y = 0; y < 3; ++y) for (int x = 0; x < 3; ++x) {
      int cx = x0 + x * cell + 8, cy = y0 + y * cell + 8;
      if (board[y * 3 + x] == 1) {
        d.drawLine(cx - 5, cy - 5, cx + 5, cy + 5, SSD1306_WHITE);
        d.drawLine(cx + 5, cy - 5, cx - 5, cy + 5, SSD1306_WHITE);
      } else if (board[y * 3 + x] == 2) d.drawCircle(cx, cy, 5, SSD1306_WHITE);
    }
    d.setCursor(0, 22); d.print(F("X YOU"));
    d.setCursor(0, 34); d.print(F("O CPU"));
    d.setCursor(0, 49); d.print(F("Move: joystick"));
    d.setCursor(0, 59); d.print(F("Place: GPIO13"));
    d.display();
  }

 private:
  uint8_t board[9] = {};
  int cursorX = 1, cursorY = 1;
  bool hard = false, stickReady = false;

  bool hasLine(int player) const {
    static const uint8_t lines[8][3] = {
      {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6},
      {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}
    };
    for (auto &line : lines) if (board[line[0]] == player && board[line[1]] == player && board[line[2]] == player) return true;
    return false;
  }

  bool full() const {
    for (auto cell : board) if (!cell) return false;
    return true;
  }

  bool wouldWin(int index, int player) {
    if (board[index]) return false;
    board[index] = player;
    bool result = hasLine(player);
    board[index] = 0;
    return result;
  }

  int cpuMove() {
    if (hard) {
      for (int i = 0; i < 9; ++i) if (wouldWin(i, 2)) return i;
      for (int i = 0; i < 9; ++i) if (wouldWin(i, 1)) return i;
      if (!board[4]) return 4;
      static const int corners[] = {0, 2, 6, 8};
      int available[4], count = 0;
      for (int i : corners) if (!board[i]) available[count++] = i;
      if (count) return available[random(count)];
    }
    int available[9], count = 0;
    for (int i = 0; i < 9; ++i) if (!board[i]) available[count++] = i;
    return count ? available[random(count)] : -1;
  }

  void placePlayer() {
    board[cursorY * 3 + cursorX] = 1;
    if (hasLine(1)) { score = 1000; won = over = true; return; }
    if (full()) { score = 100; over = true; return; }
    int move = cpuMove();
    if (move < 0) { score = 100; over = true; return; }
    board[move] = 2;
    if (hasLine(2)) { score = 0; over = true; return; }
    if (full()) { score = 100; over = true; }
  }
};
