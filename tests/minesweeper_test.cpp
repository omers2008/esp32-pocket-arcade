#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/Minesweeper.h"
#undef private

uint32_t testClock = 0;

int main() {
  std::srand(23);
  Minesweeper m;
  m.start(false);
  assert(m.width == 8 && m.height == 6 && m.mineTotal == 8 && !m.over);
  m.update(0, 0, false, false); m.update(1000, 0, false, false);
  assert(m.cursorX == 5);
  m.update(0, 0, false, false); m.update(0, -1000, false, false);
  assert(m.cursorY == 4);
  m.cursorX = 4; m.cursorY = 3;
  m.update(0, 0, true, false);
  assert(!m.firstDig && !m.mine[3][4] && m.revealed[3][4]);
  m.cursorX = 0; m.cursorY = 0; m.update(0, 0, false, true);
  assert(m.flags == 1 && m.flagged[0][0]);
  m.update(0, 0, false, true); assert(m.flags == 0 && !m.flagged[0][0]);

  m.start(true); assert(m.width == 12 && m.mineTotal == 16);
  m.firstDig = false; m.cursorX = 0; m.cursorY = 0; m.mine[0][0] = true; m.dig();
  assert(m.over && !m.won && m.score == 0 && m.revealed[0][0]);

  m.start(false); m.firstDig = false; m.cursorX = 0; m.cursorY = 0;
  for (int y = 0; y < m.height; ++y) for (int x = 0; x < m.width; ++x) {
    m.mine[y][x] = false; m.adjacent[y][x] = 0;
  }
  m.dig(); assert(m.over && m.won && m.score == 1000);
  Adafruit_SSD1306 d; m.draw(d);
  std::cout << "PASS: Minesweeper field sizes, safe first dig, movement, dig/flag actions, flood clear, and mine loss.\n";
}
