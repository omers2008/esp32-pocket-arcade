#include <Arduino.h>
#define ARCADE_TEST_TFT
#include "../ESP32_Snake/ArcadeDisplay.h"
#define private public
#include "../ESP32_Snake/Tetris.h"
#undef private
#include <cassert>
#include <iostream>
uint32_t testClock=0;
int main() {
  Tetris g;g.start(false);
  // Lock an O: all four cells retain the piece's yellow color ID.
  g.piece=1;g.pieceX=3;g.pieceY=18;g.rotation=0;g.lockPiece();
  for(int y=18;y<20;++y)for(int x=4;x<6;++x)assert(g.cellTypes[y][x]==2);
  // Clear the bottom row with an I; surviving colors must move with their cells.
  g.start(false);g.board[19]=0x3FF & ~0xF;
  for(int x=4;x<10;++x)g.cellTypes[19][x]=3;
  g.board[18]=1<<8;g.cellTypes[18][8]=7;
  g.piece=0;g.rotation=0;g.pieceX=0;g.pieceY=18;g.lockPiece();
  assert(g.lines==1 && g.board[19]==(1<<8) && g.cellTypes[19][8]==7);
  for(int x=0;x<10;++x)assert(g.cellTypes[0][x]==0);
  ArcadeDisplay d;assert(d.begin());int frames=tftTrace.transactions;g.draw(d);
  assert(tftTrace.transactions==frames+6 && tftTrace.w==320 && tftTrace.h==40);
  g.start(true);for(auto &row:g.cellTypes)for(auto c:row)assert(c==0);
  assert(g.lastClearCount==0);
  std::cout<<"PASS: Tetris locked colors, row shifts, reset and native color frame.\n";
}
