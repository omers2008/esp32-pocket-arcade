#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

class Tetris {
 public:
  uint32_t score = 0, lines = 0;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode;
    score = lines = 0;
    over = false;
    for (auto &row : board) row = 0;
    bagIndex = 7;
    heldPiece = -1;
    holdUsed = false;
    horizontal = 0;
    nextPiece = takeFromBag();
    introduceNext();
  }

  void update(int stickX, int stickY, bool rotatePressed, bool holdPressed) {
    if (over) return;
    uint32_t now = millis();

    if (holdPressed) { // Debounced GPIO14 press; joystick up has no action.
      if (!holdUsed) {
        int outgoing = piece;
        if (heldPiece < 0) introduceNext();
        else spawn(heldPiece);
        heldPiece = outgoing;
        holdUsed = true; // A swap/replacement does NOT unlock another hold.
        return;
      }
    }

    if (rotatePressed) rotateClockwise();
    int wanted = stickX > 650 ? 1 : stickX < -650 ? -1 : 0;
    if (wanted != horizontal) {
      horizontal = wanted;
      repeating = false;
      lastHorizontal = now;
      if (horizontal && fits(piece, rotation, pieceX + horizontal, pieceY)) pieceX += horizontal;
    } else if (horizontal && now - lastHorizontal >= (repeating ? 85u : 220u)) {
      repeating = true;
      lastHorizontal = now;
      if (fits(piece, rotation, pieceX + horizontal, pieceY)) pieceX += horizontal;
    }

    int level = min(int(lines / 10), 10);
    uint32_t fallMs = hard ? max(60, 360 - level * 30) : max(100, 650 - level * 50);
    bool softDrop = stickY < -650;
    if (softDrop) fallMs = hard ? 25 : 35;
    if (now - lastFall >= fallMs) {
      lastFall = now;
      if (fits(piece, rotation, pieceX, pieceY + 1)) {
        ++pieceY;
        if (softDrop) ++score;
      }
    }

    if (fits(piece, rotation, pieceX, pieceY + 1)) {
      grounded = false;
    } else {
      if (!grounded) { grounded = true; groundedAt = now; }
      if (now - groundedAt >= (hard ? 250u : 450u)) lockPiece();
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay();
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    // 10 x 20 cells, three pixels per cell, with room for side panels.
    d.drawRect(45, 1, 32, 62, SSD1306_WHITE);
    for (int y = 0; y < 20; ++y) {
      for (int x = 0; x < 10; ++x) {
        if (board[y] & (1u << x)) drawCell(d, x, y, false);
      }
    }
    int ghostY = pieceY;
    while (fits(piece, rotation, pieceX, ghostY + 1)) ++ghostY;
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
        if (!block(piece, rotation, x, y)) continue;
        drawCell(d, pieceX + x, ghostY + y, true);
        drawCell(d, pieceX + x, pieceY + y, false);
      }
    }
    d.setCursor(2, 2); d.print(F("HOLD"));
    d.drawRect(2, 12, 30, 20, SSD1306_WHITE);
    if (heldPiece >= 0) preview(d, heldPiece, 8, 16, 3);
    d.setCursor(2, 34); d.print(holdUsed ? F("USED") : F("14"));
    d.setCursor(2, 45); d.print(F("NEXT"));
    preview(d, nextPiece, 8, 55, 2);
    d.setCursor(82, 2); d.print(F("SCORE"));
    d.setCursor(82, 12); d.print(score);
    d.setCursor(82, 27); d.print(F("LINES"));
    d.setCursor(82, 37); d.print(lines);
    d.setCursor(82, 53); d.print(hard ? F("HARD") : F("EASY"));
    d.display();
  }

 private:
  uint16_t board[20] = {};
  uint8_t bag[7] = {}, bagIndex = 7;
  int piece = 0, nextPiece = 0, heldPiece = -1;
  int rotation = 0, pieceX = 3, pieceY = -1, horizontal = 0;
  bool hard = false, holdUsed = false;
  bool grounded = false, repeating = false;
  uint32_t lastFall = 0, groundedAt = 0, lastHorizontal = 0;

  static bool block(int type, int turn, int x, int y) {
    // I, O, T, S, Z, J, L. O stays fixed; the others turn clockwise.
    static const uint16_t shapes[7] = {0x0F00, 0x6600, 0x4E00, 0x6C00, 0xC600, 0x8E00, 0x2E00};
    int size = type == 0 || type == 1 ? 4 : 3;
    if (x < 0 || y < 0 || x >= size || y >= size) return false;
    if (type != 1) {
      for (int r = 0; r < turn; ++r) {
        int oldX = x;
        x = y;
        y = size - 1 - oldX;
      }
    }
    return (shapes[type] & (0x8000u >> (y * 4 + x))) != 0;
  }

  bool fits(int type, int turn, int px, int py) const {
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
        if (!block(type, turn, x, y)) continue;
        int bx = px + x, by = py + y;
        if (bx < 0 || bx >= 10 || by >= 20) return false;
        if (by >= 0 && (board[by] & (1u << bx))) return false;
      }
    }
    return true;
  }

  int takeFromBag() {
    if (bagIndex == 7) {
      for (int i = 0; i < 7; ++i) bag[i] = i;
      for (int i = 6; i > 0; --i) {
        int j = random(i + 1);
        uint8_t saved = bag[i]; bag[i] = bag[j]; bag[j] = saved;
      }
      bagIndex = 0;
    }
    return bag[bagIndex++];
  }

  void spawn(int type) {
    piece = type;
    rotation = 0;
    pieceX = 3; pieceY = -1;
    grounded = false;
    lastFall = millis();
    if (!fits(piece, rotation, pieceX, pieceY)) over = true;
  }

  void introduceNext() {
    spawn(nextPiece);
    nextPiece = takeFromBag();
  }

  void rotateClockwise() {
    if (piece == 1) return;
    int nextRotation = (rotation + 1) % 4;
    static const int kicks[] = {0, -1, 1, -2, 2};
    for (int lift = 0; lift <= 2; ++lift) {
      for (int offset : kicks) {
        if (fits(piece, nextRotation, pieceX + offset, pieceY - lift)) {
          pieceX += offset; pieceY -= lift; rotation = nextRotation;
          return;
        }
      }
    }
  }

  void lockPiece() {
    // A piece locking above the visible board ends the run.
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
        if (block(piece, rotation, x, y) && pieceY + y < 0) { over = true; return; }
      }
    }
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
        if (block(piece, rotation, x, y)) board[pieceY + y] |= 1u << (pieceX + x);
      }
    }
    int cleared = 0;
    for (int y = 19; y >= 0;) {
      if (board[y] == 0x3FF) {
        for (int row = y; row > 0; --row) board[row] = board[row - 1];
        board[0] = 0;
        ++cleared;
      } else --y;
    }
    static const uint16_t rewards[] = {0, 100, 300, 500, 800};
    score += rewards[cleared] * (lines / 10 + 1);
    lines += cleared;
    holdUsed = false; // Only locking a piece allows hold on the next turn.
    introduceNext();
  }

  static void drawCell(Adafruit_SSD1306 &d, int x, int y, bool ghost) {
    if (y < 0 || y >= 20 || x < 0 || x >= 10) return;
    if (ghost) d.drawPixel(47 + x * 3, 3 + y * 3, SSD1306_WHITE);
    else d.fillRect(46 + x * 3, 2 + y * 3, 2, 2, SSD1306_WHITE);
  }

  static void preview(Adafruit_SSD1306 &d, int type, int px, int py, int size) {
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
        if (block(type, 0, x, y)) d.fillRect(px + x * size, py + y * size, size - 1, size - 1, SSD1306_WHITE);
      }
    }
  }
};
