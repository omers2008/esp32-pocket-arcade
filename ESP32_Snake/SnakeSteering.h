#pragma once
#include <stdint.h>

enum Direction : uint8_t { UP, DOWN, LEFT, RIGHT };

// Four-way input with axis hysteresis and a short settling period.
class SnakeSteering {
 public:
  void reset() { active = false; }

  bool read(int x, int y, uint32_t now, Direction &wanted) {
    if (x == 0 && y == 0) {
      reset();
      return false;
    }
    int ax = x < 0 ? -x : x;
    int ay = y < 0 ? -y : y;
    bool horizontal = ax > ay;
    if (active) {
      horizontal = candidate == LEFT || candidate == RIGHT;
      // Crossing a diagonal must clearly favor the new axis, rather than
      // switching each time two nearly equal readings trade places.
      if (horizontal && (ax == 0 || ay > ax * 3 / 2 + 150)) horizontal = false;
      else if (!horizontal && (ay == 0 || ax > ay * 3 / 2 + 150)) horizontal = true;
    }
    Direction next = horizontal ? (x > 0 ? RIGHT : LEFT) : (y > 0 ? UP : DOWN);
    if (!active || next != candidate) {
      candidate = next;
      changedAt = now;
      active = true;
      return false;
    }
    if (now - changedAt < 25) return false;
    wanted = candidate;
    return true;
  }

 private:
  bool active = false;
  Direction candidate = RIGHT;
  uint32_t changedAt = 0;
};
