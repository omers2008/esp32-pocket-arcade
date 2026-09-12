#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/DinoRunner.h"
#undef private
uint32_t testClock = 0;
void tick(DinoRunner &g, int y = 0, bool jump = false, bool duck = false) {
  testClock += 20; g.update(y, jump, duck);
}
void begin(DinoRunner &g, bool hard = false) {
  g.start(hard); tick(g, 0, true); g.spawnDistance = 100000;
}
int main() {
  DinoRunner g; Adafruit_SSD1306 d;
  g.start(false); tick(g); assert(!g.started && g.score == 0); g.draw(d);
  tick(g, 0, true); assert(g.started && g.grounded);
  g.update(0, true, false); tick(g); assert(!g.grounded && g.feet < 58);
  float v = g.velocity; tick(g, 0, true); assert(g.velocity > v);
  for (int i = 0; i < 40; ++i) tick(g);
  assert(g.grounded && g.feet == 58 && g.velocity == 0);
  begin(g); tick(g, 1000); assert(!g.grounded);
  for (int i = 0; i < 50; ++i) tick(g, 1000);
  assert(g.grounded); // Holding UP cannot repeat the jump.
  tick(g); tick(g, 1000); assert(!g.grounded);
  begin(g); tick(g, 0, true, true); assert(g.ducking && g.grounded);
  tick(g, -1000); assert(g.ducking); tick(g); assert(!g.ducking);

  // Each obstacle must have a usable jump window at starting and maximum speeds.
  for (int hard = 0; hard < 2; ++hard) for (int fast = 0; fast < 2; ++fast) {
    for (int kind : {0, 1, 2, 4}) {
      int safeTimings = 0;
      for (int delay = 0; delay < 80; ++delay) {
        begin(g, hard); g.score = fast ? 2000 : 0;
        int w = kind == 2 ? 16 : kind == 4 ? 14 : kind == 1 ? 8 : 7;
        int h = kind == 4 ? 8 : kind == 1 ? 16 : 11;
        g.obstacles[0] = {115, kind == 4 ? 48 : 58 - h, w, h, kind, true};
        for (int i = 0; i < 110 && !g.over; ++i) tick(g, 0, i == delay);
        if (!g.over) ++safeTimings;
      }
      assert(safeTimings >= 4);
    }
    begin(g, hard); g.obstacles[0] = {85, 39, 14, 8, 3, true};
    for (int i = 0; i < 60 && !g.over; ++i) tick(g, 0, false, true);
    assert(!g.over); // Duck under a bird.
    begin(g, hard); g.obstacles[0] = {40, 39, 14, 8, 3, true};
    for (int i = 0; i < 20; ++i) tick(g);
    assert(g.over); // Standing collides with that same bird.
    uint32_t score = g.score; tick(g, 0, true); assert(g.over && g.score == score);
    begin(g, hard); g.obstacles[0] = {40, 48, 14, 8, 4, true};
    for (int i = 0; i < 20; ++i) tick(g, 0, false, true);
    assert(g.over); // Low birds require jumping.
  }

  // Play actual generated sequences with a simple jump/duck policy. This checks
  // spacing and speed progression together, not just isolated obstacles.
  std::srand(99);
  int kinds = 0;
  for (int hard = 0; hard < 2; ++hard) {
    begin(g, hard); g.spawnDistance = 0;
    for (int i = 0; i < 16000; ++i) {
      bool jump = false, duck = false;
      for (auto &o : g.obstacles) if (o.active) {
        kinds |= 1 << o.kind;
        float gap = o.x - 29;
        if (o.x + o.w > 20 && gap < g.speed() * 8) {
          if (o.kind == 3) duck = true;
          else if (g.grounded) jump = true;
        }
      }
      tick(g, 0, jump, duck); g.draw(d);
      assert(!g.over && g.feet >= 20 && g.feet <= 58);
      assert(g.speed() <= (hard ? 2.9f : 2.6f));
    }
    assert(g.score > 2000);
  }
  assert(kinds == 31);
  testClock = UINT32_MAX - 10; begin(g); tick(g, 0, true);
  assert(!g.grounded && !g.over);
  g.start(true); assert(g.score == 0 && !g.over && !g.started && g.grounded);
  for (auto &o : g.obstacles) assert(!o.active);
  std::cout << "PASS: buffered jump, no double/held jumps, ducking, all obstacle clearances, collision, speed caps, reset/rollover, 32000 generated-run frames.\n";
}
