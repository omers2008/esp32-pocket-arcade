#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <cassert>
#include <iostream>
#define private public
#include "../ESP32_Snake/PacMan.h"
#undef private
uint32_t testClock = 0;
void tick(PacMan &g, int x = 0, int y = 0, uint32_t elapsed = 140) {
  testClock += elapsed; g.update(x, y);
}
int main() {
  std::srand(42);
  PacMan g;
  for (bool hard : {false, true}) {
    g.start(hard);
    assert(g.lives == 3 && !g.over && g.level == 1 && g.ghostCount() == (hard ? 3 : 2));
    int dist[PacMan::H][PacMan::W]; g.distances(dist);
    int count = 0, powers = 0;
    for (int y = 0; y < PacMan::H; ++y) for (int x = 0; x < PacMan::W; ++x) {
      if (PacMan::walkable(x, y)) assert(dist[y][x] < PacMan::W * PacMan::H);
      if (g.pellets[y][x]) { ++count; assert(PacMan::walkable(x, y)); }
      if (g.pellets[y][x] == 2) ++powers;
    }
    assert(count == g.remaining && powers == 4);
    for (int i = 0; i < 3; ++i) assert(PacMan::walkable(g.ghosts[i].x, g.ghosts[i].y));
    tick(g, 1000, 0, 700); assert(g.px == 10); // READY freeze
    g.direction = 1; tick(g, 0, 1000, 50); // Queue up while still blocked.
    assert(g.px == 11 && g.py == 9 && g.wanted == 0 && g.score == 10);
    tick(g); assert(g.px == 11 && g.py == 8 && g.direction == 0); // buffered turn
    tick(g); assert(g.px == 11 && g.py == 7); // neutral keeps moving
    tick(g, 0, -1000); assert(g.py == 8); // immediate reversal
    // Wall stop without leaving the maze.
    g.px = 1; g.py = 1; g.direction = 3; g.wanted = 3;
    tick(g); assert(g.px == 1 && g.py == 1 && g.powered);
    uint32_t before = g.score; int remaining = g.remaining;
    g.eatPellet(); assert(g.score == before && g.remaining == remaining);
    g.ghosts[0].x = g.px; g.ghosts[0].y = g.py;
    assert(!g.collide() && !g.ghosts[0].active && g.score == before + 200);
    g.ghosts[1].x = g.px; g.ghosts[1].y = g.py;
    assert(!g.collide() && g.score == before + 600);
    // Expiration uses elapsed time, even without a movement frame.
    testClock = g.powerAt + g.powerDuration();
    g.lastPlayer = g.lastGhost = testClock; g.update(0, 0); assert(!g.powered);
    // Frightened ghosts move away using maze distance, not through walls.
    g.start(hard); testClock += 1900;
    g.powered = true; g.powerAt = testClock; g.lastPlayer = testClock;
    g.ghosts[0] = {14, 9, -1, true, 0};
    g.update(0, 0); assert(g.ghosts[0].x == 15);
    // Eaten ghost waits before returning home.
    g.ghosts[0].active = false; g.ghosts[0].eatenAt = testClock;
    tick(g, 0, 0, 2100); assert(!g.ghosts[0].active);
    tick(g, 0, 0, 300); assert(g.ghosts[0].active && g.ghosts[0].x == PacMan::homeX(0));
    // Collision on the player's move catches opposing cell swaps.
    g.start(hard); testClock += 1900;
    g.ghosts[0] = {11, 9, 3, true, 0};
    g.update(1000, 0);
    assert(g.lives == 2 && g.px == 10 && g.py == 9 && !g.over);
    assert(g.pellets[9][11] == 0); // Losing a life does not reset eaten pellets.
    g.ghosts[0].x = 10; g.ghosts[0].y = 9;
    assert(!g.collide() && g.lives == 2); // Respawn grace.
    for (int life = 2; life > 0; --life) {
      testClock += 1900; g.ghosts[0].x = g.px; g.ghosts[0].y = g.py;
      assert(g.collide() && g.lives == life - 1);
    }
    assert(g.over); tick(g, 1000); assert(g.lives == 0);
    // Eating the last pellet advances a round and resets actors/power.
    g.start(hard); testClock += 800;
    for (auto &row : g.pellets) for (auto &p : row) p = 0;
    g.pellets[9][11] = 1; g.remaining = 1;
    g.update(1000, 0);
    assert(g.level == 2 && g.score == 510 && g.remaining > 1 && g.lives == 3);
    assert(g.px == 10 && !g.powered && g.direction == -1);
    Adafruit_SSD1306 screen;
    for (int i = 0; i < 20000; ++i) {
      tick(g, (std::rand() % 3 - 1) * 1000, (std::rand() % 3 - 1) * 1000);
      assert(PacMan::walkable(g.px, g.py));
      for (auto &ghost : g.ghosts) assert(PacMan::walkable(ghost.x, ghost.y));
      assert(g.lives >= 0 && g.lives <= 3 && g.remaining >= 0);
      g.draw(screen);
      if (g.over) g.start(hard);
    }
  }
  testClock = UINT32_MAX - 400; g.start(false);
  tick(g, 1000, 0, 800); assert(g.px == 11 && !g.over);
  std::cout << "PASS: Pac-Man maze connectivity, pellets, power, queued turns, walls, chasing/fleeing, collisions, respawn, rounds, timer rollover, 40000 input/draw steps.\n";
}
