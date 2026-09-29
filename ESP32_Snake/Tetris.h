#pragma once
#include <Arduino.h>
#include "ArcadeDisplay.h"

class Tetris {
 public:
  uint32_t score = 0, lines = 0;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode;
    score = lines = 0;
    over = false;
    for (auto &row : board) row = 0;
    for (auto &row : cellTypes) for (auto &cell : row) cell = 0;
    lastClearCount = 0;
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

  void draw(ArcadeDisplay &d) {
#if defined(ARDUINO) || defined(ARCADE_TEST_TFT)
    if (d.colorFrame([&](Adafruit_GFX &canvas) { drawColor(canvas); })) return;
#endif
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
  uint8_t cellTypes[20][10] = {}; // Piece ID + 1; preserve colors after locking.
  int lastClearCount = 0;
  uint32_t lastClearAt = 0;
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
        if (block(piece, rotation, x, y)) {
          board[pieceY + y] |= 1u << (pieceX + x);
          cellTypes[pieceY + y][pieceX + x] = piece + 1;
        }
      }
    }
    int cleared = 0;
    for (int y = 19; y >= 0;) {
      if (board[y] == 0x3FF) {
        for (int row = y; row > 0; --row) {
          board[row] = board[row - 1];
          for (int col = 0; col < 10; ++col) cellTypes[row][col] = cellTypes[row - 1][col];
        }
        board[0] = 0;
        for (auto &cell : cellTypes[0]) cell = 0;
        ++cleared;
      } else --y;
    }
    static const uint16_t rewards[] = {0, 100, 300, 500, 800};
    score += rewards[cleared] * (lines / 10 + 1);
    lines += cleared;
    if (cleared) { lastClearCount = cleared; lastClearAt = millis(); }
    holdUsed = false; // Only locking a piece allows hold on the next turn.
    introduceNext();
  }

  static void drawCell(ArcadeDisplay &d, int x, int y, bool ghost) {
    if (y < 0 || y >= 20 || x < 0 || x >= 10) return;
    if (ghost) d.drawPixel(47 + x * 3, 3 + y * 3, SSD1306_WHITE);
    else d.fillRect(46 + x * 3, 2 + y * 3, 2, 2, SSD1306_WHITE);
  }

  static void preview(ArcadeDisplay &d, int type, int px, int py, int size) {
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
        if (block(type, 0, x, y)) d.fillRect(px + x * size, py + y * size, size - 1, size - 1, SSD1306_WHITE);
      }
    }
  }

#if defined(ARDUINO) || defined(ARCADE_TEST_TFT)
  static uint16_t pieceColor(int type) {
    static const uint16_t colors[] = {0x2E9F,0xFFE6,0xB2FF,0x5F54,0xF9C7,0x3B7F,0xFD28};
    return type >= 0 && type < 7 ? colors[type] : 0x8CB3;
  }
  static void tile(Adafruit_GFX &d,int x,int y,int type,int size,bool ghost=false) {
    uint16_t c=pieceColor(type);
    if(ghost) { d.drawRect(x+1,y+1,size-2,size-2,c);return; }
    d.fillRect(x+1,y+1,size-1,size-1,c);
    d.drawFastHLine(x+2,y+2,size-3,0xFFFF);
    d.drawFastVLine(x+2,y+3,size-4,0xCE79);
    d.drawFastHLine(x+2,y+size-1,size-2,0x2945);
  }
  static void colorPreview(Adafruit_GFX &d,int type,int top) {
    if(type<0) { ColorMenu::label(d,34,top+22,"EMPTY",ColorMenu::MUTED);return; }
    int minX=4,maxX=0,minY=4,maxY=0;
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)if(block(type,0,x,y)) {
      minX=min(minX,x);maxX=max(maxX,x);minY=min(minY,y);maxY=max(maxY,y);
    }
    int startX=10+(86-(maxX-minX+1)*12)/2,startY=top+(48-(maxY-minY+1)*12)/2;
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)if(block(type,0,x,y))
      tile(d,startX+(x-minX)*12,startY+(y-minY)*12,type,12);
  }
  void drawColor(Adafruit_GFX &d) {
    using namespace ColorMenu;
    d.fillScreen(BG);d.setTextWrap(false);
    label(d,10,12,"TETRIS",CYAN,2);
    label(d,12,40,"HOLD",holdUsed?MUTED:CYAN);
    d.fillRoundRect(10,51,86,48,5,CARD);colorPreview(d,heldPiece,51);
    label(d,15,105,holdUsed?"USED THIS TURN":"14: SWAP",holdUsed?MUTED:WHITE);
    label(d,12,125,"NEXT",CYAN);
    d.fillRoundRect(10,136,86,48,5,CARD);colorPreview(d,nextPiece,136);
    label(d,12,199,"13 ROTATE",WHITE);label(d,12,213,"DOWN: FASTER",MUTED);label(d,12,227,"12 MENU",MUTED);
    label(d,224,20,"SCORE",CYAN);d.setTextSize(score<10000000?2:1);d.setTextColor(WHITE);d.setCursor(224,36);d.print(score);
    label(d,224,72,"LINES",CYAN);d.setTextSize(2);d.setTextColor(WHITE);d.setCursor(224,87);d.print(min(lines,uint32_t(9999999)));
    label(d,224,119,"LEVEL",CYAN);d.setTextSize(2);d.setTextColor(WHITE);d.setCursor(224,134);d.print(lines/10+1);
    label(d,224,167,hard?"HARD":"EASY",hard?0xFD28:0x5F54,2);
    bool clearFlash=lastClearCount && uint32_t(millis()-lastClearAt)<700;
    if(clearFlash)label(d,224,198,lastClearCount==4?"TETRIS!":"LINE CLEAR",GOLD);
    d.fillRect(109,19,102,202,0x0000);
    d.drawRect(108,18,104,204,clearFlash?GOLD:0x3A50);
    for(int gy=0;gy<20;++gy)for(int gx=0;gx<10;++gx) {
      d.drawPixel(110+gx*10,20+gy*10,0x18C3);
      if(board[gy]&(1u<<gx))tile(d,110+gx*10,20+gy*10,int(cellTypes[gy][gx])-1,10);
    }
    int ghostY=pieceY;while(fits(piece,rotation,pieceX,ghostY+1))++ghostY;
    for(int gy=0;gy<4;++gy)for(int gx=0;gx<4;++gx)if(block(piece,rotation,gx,gy)) {
      if(ghostY+gy>=0 && ghostY+gy<20)tile(d,110+(pieceX+gx)*10,20+(ghostY+gy)*10,piece,10,true);
    }
    for(int gy=0;gy<4;++gy)for(int gx=0;gx<4;++gx)if(block(piece,rotation,gx,gy) && pieceY+gy>=0 && pieceY+gy<20)
      tile(d,110+(pieceX+gx)*10,20+(pieceY+gy)*10,piece,10);
    label(d,125,229,"POCKET BLOCKS",MUTED);
  }
#endif
};
