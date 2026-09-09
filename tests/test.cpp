#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
// Expose board fixtures, while exercising the real firmware's game methods.
#define private public
#include "../ESP32_Snake/Tetris.h"
#undef private
uint32_t testClock = 0;

void tick(Tetris &g, int x, int y, bool rotate = false, uint32_t elapsed = 1, bool hold = false) {
  testClock += elapsed;
  g.update(x, y, rotate, hold);
}

void land(Tetris &g) {
  while (g.fits(g.piece, g.rotation, g.pieceX, g.pieceY + 1)) ++g.pieceY;
  g.lockPiece();
}

int main() {
  std::srand(42);
  for (int type = 0; type < 7; ++type) {
    for (int turn = 0; turn < 4; ++turn) {
      int cells = 0;
      for (int y = 0; y < 4; ++y) for (int x = 0; x < 4; ++x) cells += Tetris::block(type, turn, x, y);
      assert(cells == 4);
    }
  }
  Tetris bag;
  for (int round = 0; round < 20; ++round) {
    int mask = 0;
    for (int i = 0; i < 7; ++i) mask |= 1 << bag.takeFromBag();
    assert(mask == 127);
  }

  Tetris g;
  g.start(false);
  tick(g, 0, 1000); // Joystick up must never store a piece.
  assert(g.heldPiece == -1);
  tick(g, 0, 0);
  int first = g.piece, next = g.nextPiece;
  tick(g, 0, 0, false, 1, true);
  assert(g.heldPiece == first && g.piece == next && g.holdUsed);
  tick(g, 0, 0);
  tick(g, 0, 0, false, 1, true); // A new button press cannot reuse hold this turn.
  assert(g.heldPiece == first && g.piece == next);
  land(g);
  assert(!g.over && !g.holdUsed);
  int newlySpawned = g.piece;
  tick(g, 0, 1000); // No new button edge across a lock: no auto-swap.
  assert(g.piece == newlySpawned && g.heldPiece == first);
  tick(g, 0, 0);
  tick(g, 0, 0, false, 1, true);
  assert(g.piece == first && g.heldPiece == newlySpawned && g.holdUsed && g.rotation == 0);

  Tetris clear;
  clear.start(false);
  for (int row = 16; row < 20; ++row) clear.board[row] = 0x3FF ^ (1 << 4);
  clear.piece = 0; clear.rotation = 1; clear.pieceX = 2; clear.pieceY = 16;
  assert(clear.fits(0, 1, 2, 16));
  clear.lockPiece();
  assert(clear.lines == 4 && clear.score == 800 && !clear.over);
  for (auto row : clear.board) assert(row == 0);

  Tetris kick;
  kick.start(false);
  kick.piece = 0; kick.rotation = 1; kick.pieceX = -2; kick.pieceY = 4;
  assert(kick.fits(0, 1, -2, 4));
  kick.rotateClockwise();
  assert(kick.rotation == 2 && kick.fits(kick.piece, kick.rotation, kick.pieceX, kick.pieceY));

  Tetris slow, fast;
  testClock = 0; slow.start(false); fast.start(false);
  testClock = 40;
  slow.update(0, 0, false, false); fast.update(0, -1000, false, false);
  assert(slow.pieceY == -1 && fast.pieceY == 0 && fast.score == 1);
  Tetris easy, hard;
  testClock = 0; easy.start(false); hard.start(true);
  testClock = 400;
  easy.update(0, 0, false, false); hard.update(0, 0, false, false);
  assert(easy.pieceY == -1 && hard.pieceY == 0);

  Tetris top;
  top.start(false); top.board[0] = 0x3FF; top.spawn(2);
  assert(top.over);

  // Long deterministic input runs exercise board bounds, locking and swapping.
  int transitions = 0;
  for (int difficulty = 0; difficulty < 2; ++difficulty) {
    Tetris run; run.start(difficulty != 0);
    for (int i = 0; i < 10000; ++i) {
      tick(run, (std::rand() % 3 - 1) * 1000, (std::rand() % 3 - 1) * 1000, std::rand() % 8 == 0, 35, std::rand() % 12 == 0);
      assert(run.heldPiece >= -1 && run.heldPiece < 7);
      for (auto row : run.board) assert((row & ~0x3FF) == 0);
      if (run.over) { ++transitions; run.start(difficulty != 0); }
      else assert(run.fits(run.piece, run.rotation, run.pieceX, run.pieceY));
    }
  }
  assert(transitions > 0);
  std::cout << "PASS: rotations, seven-bag, hold/swap gating, four-line clear, wall kick, soft drop, difficulty, top-out, 20000 input steps.\n";
}
