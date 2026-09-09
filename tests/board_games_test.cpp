#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/FourInRow.h"
#include "../ESP32_Snake/TicTacToe.h"
#undef private

uint32_t testClock = 0;

int main() {
  std::srand(19);
  FourInRow four;
  four.start(false);
  assert(!four.over && four.selectedCol == 3 && four.landingRow(0) == 5);
  four.update(0, false); four.update(1000, false); assert(four.selectedCol == 4);
  four.update(0, false); four.update(-1000, false); assert(four.selectedCol == 3);
  four.board[5][0] = four.board[5][1] = four.board[5][2] = 1;
  four.selectedCol = 3; four.dropPlayer();
  assert(four.over && four.won && four.score == 1000);

  four.start(true);
  four.board[5][0] = four.board[5][1] = four.board[5][2] = 1;
  four.selectedCol = 6; four.dropPlayer();
  assert(!four.over && four.board[5][3] == 2); // Hard CPU blocks the open four.
  four.start(true);
  four.board[5][0] = four.board[5][1] = four.board[5][2] = 2;
  assert(four.wouldWin(3, 2));

  TicTacToe ttt;
  ttt.start(false);
  ttt.update(0, 0, false); ttt.update(1000, 0, false); assert(ttt.cursorX == 2);
  ttt.update(0, 0, false); ttt.update(0, -1000, false); assert(ttt.cursorY == 2);
  ttt.start(false);
  ttt.board[0] = ttt.board[1] = 1; ttt.cursorX = 2; ttt.cursorY = 0; ttt.placePlayer();
  assert(ttt.over && ttt.won && ttt.score == 1000);
  ttt.start(true);
  ttt.board[0] = ttt.board[1] = 1; ttt.cursorX = 0; ttt.cursorY = 1; ttt.placePlayer();
  assert(!ttt.over && ttt.board[2] == 2); // Hard CPU blocks the top row.
  ttt.start(true);
  ttt.board[0] = ttt.board[1] = 2; assert(ttt.wouldWin(2, 2));

  Adafruit_SSD1306 d; four.draw(d); ttt.draw(d);
  std::cout << "PASS: 4 In A Row and Tic-Tac-Toe controls, placement, wins, draws, and CPU tactics.\n";
}
