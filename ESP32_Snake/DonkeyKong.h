#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Compact barrel-stage adaptation: five sloped girders, ladders, rescue,
// hammers and the oil fire. Geometry is shared by drawing and movement.
class DonkeyKong {
 public:
  uint32_t score = 0;
  int lives = 3, round = 1;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode; lives = hard ? 2 : 3; round = 1; score = 0; over = false;
    started = jumpQueued = lastJump = false; cleared = 0;
    resetBoard();
  }
  void update(int sx, int sy, bool jumpPressed) {
    if (over) return;
    // Accept held or edge inputs and retain a short tap between physics frames.
    bool edge = jumpPressed && !lastJump; lastJump = jumpPressed;
    if (!started) { if (edge) { started = true; lastFrame = millis(); } return; }
    jumpQueued |= edge;
    uint32_t now = millis();
    if (now - lastFrame < 20) return;
    lastFrame = now;
    if (cleared) {
      jumpQueued = false;
      if (--cleared == 0) { round = min(round + 1, 99); resetBoard(); }
      return;
    }
    tick(sx, sy, jumpQueued); jumpQueued = false;
  }
  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    if (!started) {
      line(d, 0, "DONKEY KONG"); line(d, 11, "Stick: move / climb");
      line(d, 22, "13: jump barrels"); line(d, 33, "Hammers smash barrels");
      line(d, 44, "Rescue at top right"); line(d, 55, "13: start");
      d.display(); return;
    }
    if (cleared) {
      line(d, 4, "RESCUED!");
      d.setCursor(0, 19); d.print(F("ROUND ")); d.print(round); d.print(F(" CLEAR"));
      d.setCursor(0, 33); d.print(F("SCORE ")); d.print(score);
      line(d, 49, "Next round..."); d.display(); return;
    }
    for (int level = 0; level < 5; ++level) {
      int y1 = int(floorY(level, 3)), y2 = int(floorY(level, 124));
      d.drawLine(3, y1, 124, y2, SSD1306_WHITE);
      for (int px = 7; px < 123; px += 10) d.drawPixel(px, int(floorY(level, px)) + 1, SSD1306_WHITE);
    }
    for (int level = 0; level < 4; ++level) for (int n = 0; n < 2; ++n) {
      int lx = ladderX(level, n), bottom = int(floorY(level, lx)), top = int(floorY(level+1, lx));
      d.drawFastVLine(lx-2, top, bottom-top, SSD1306_WHITE);
      d.drawFastVLine(lx+2, top, bottom-top, SSD1306_WHITE);
      for (int py = top+2; py < bottom; py += 3) d.drawFastHLine(lx-1, py, 3, SSD1306_WHITE);
    }
    gorilla(d);
    // Rescue character and a small heart on the upper girder.
    int ry = int(floorY(4, 114));
    d.fillRect(113, ry-7, 3, 2, SSD1306_WHITE);
    d.drawLine(114, ry-5, 111, ry-1, SSD1306_WHITE); d.drawLine(114, ry-5, 117, ry-1, SSD1306_WHITE);
    d.drawPixel(121, ry-7, SSD1306_WHITE); d.drawPixel(123, ry-7, SSD1306_WHITE); d.drawPixel(122, ry-6, SSD1306_WHITE);
    for (int i = 0; i < 2; ++i) if (hammers[i]) drawHammer(d, hammerX(i), int(floorY(hammerLevel(i), hammerX(i)))-5);
    for (const auto &b : barrels) if (b.active) {
      d.drawCircle(int(b.x), int(b.y)-2, 2, SSD1306_WHITE);
      d.drawLine(int(b.x)-1, int(b.y)-3+(frames/4)%2, int(b.x)+1, int(b.y)-2, SSD1306_WHITE);
    }
    int oilY = int(floorY(0, 7));
    d.drawRect(4, oilY-5, 7, 5, SSD1306_WHITE);
    d.drawLine(5, oilY-6, 6, oilY-8, SSD1306_WHITE); d.drawLine(6, oilY-8, 8, oilY-6, SSD1306_WHITE);
    int fireY = int(floorY(0, fireX));
    d.drawLine(int(fireX)-2, fireY-1, int(fireX), fireY-5, SSD1306_WHITE);
    d.drawLine(int(fireX), fireY-5, int(fireX)+2, fireY-1, SSD1306_WHITE);
    if (!immune || frames%8 < 4) {
      int px = int(x), py = int(y);
      d.drawFastHLine(px-2, py-6, 5, SSD1306_WHITE); // Cap.
      d.drawFastHLine(px-1, py-5, 3, SSD1306_WHITE);
      d.drawFastVLine(px, py-4, 3, SSD1306_WHITE);
      d.drawLine(px-2, py-3, px+2, py-3, SSD1306_WHITE);
      d.drawPixel(px-1, py-1, SSD1306_WHITE); d.drawPixel(px+1, py-1, SSD1306_WHITE);
      if (hammer) drawHammer(d, px + facing * 5, py - (frames%16 < 8 ? 6 : 3));
    }
    d.fillRect(0, 0, 128, 9, SSD1306_BLACK);
    d.setCursor(0, 0); d.print(score);
    d.setCursor(43, 0); d.print(F("L")); d.print(lives);
    d.setCursor(62, 0); d.print(F("R")); d.print(round);
    d.setCursor(92, 0);
    if (hammer) { d.print(F("H")); d.print((hammer+49)/50); }
    else { d.print(F("T")); d.print((timeLeft+49)/50); }
    d.display();
  }

 private:
  struct Barrel { float x, y, vy; int level; bool active, falling, jumped; };
  Barrel barrels[10] = {};
  float x = 18, y = 58, vy = 0, fireX = 30;
  int level = 0, facing = 1, fireDir = 1, immune = 0, hammer = 0;
  int climbTo = -1, spawnTimer = 0, timeLeft = 6000, cleared = 0;
  bool jumping = false, hammers[2] = {}, hard = false, started = false;
  bool jumpQueued = false, lastJump = false;
  uint32_t lastFrame = 0, frames = 0;
  static float floorY(int l, float px) { return 59.0f - l*10 + (l%2 ? -1 : 1) * ((px-64)/32.0f); }
  static int direction(int l) { return l%2 ? -1 : 1; }
  static int ladderX(int l, int n) {
    static const int ladders[4][2] = {{111, 68}, {18, 46}, {106, 77}, {26, 55}};
    return ladders[l][n];
  }
  static int hammerLevel(int i) { return i == 0 ? 1 : 3; }
  static int hammerX(int i) { return i == 0 ? 91 : 43; }
  void resetBoard() {
    for (auto &b : barrels) b = {};
    hammers[0] = hammers[1] = true;
    x = 18; level = 0; y = floorY(0,x); vy = 0;
    jumping = false; climbTo = -1; hammer = 0; immune = 90;
    fireX = 35; fireDir = 1; facing = 1; spawnTimer = 55;
    timeLeft = hard ? 4500 : 6000; frames = 0; lastFrame = millis(); jumpQueued = false;
  }
  void loseLife() {
    if (--lives <= 0) { lives = 0; over = true; return; }
    resetBoard();
  }
  void spawn() {
    for (auto &b : barrels) if (!b.active) {
      b = {23, floorY(4,23), 0, 4, true, false, false}; break;
    }
    spawnTimer = max(65, (hard ? 110 : 165) - (round-1)*9) + int(random(20));
  }
  void roll(Barrel &b) {
    if (!b.active) return;
    if (b.falling) {
      b.vy += 0.10f; b.y += b.vy;
      if (b.y >= floorY(b.level,b.x)) { b.y = floorY(b.level,b.x); b.falling = false; b.vy = 0; }
      return;
    }
    float oldX = b.x;
    b.x += direction(b.level) * min(1.15f, (hard ? 0.72f : 0.52f) + (round-1)*0.045f);
    b.y = floorY(b.level,b.x);
    if (b.x > 123 || b.x < 4) {
      if (!b.level) { b.active = false; return; }
      b.x = constrain(b.x, 4.0f, 123.0f); --b.level; b.falling = true; b.jumped = false; return;
    }
    if (b.level > 0) for (int n = 0; n < 2; ++n) {
      int lx = ladderX(b.level-1,n);
      if ((oldX-lx)*(b.x-lx) <= 0 && random(100) < (hard ? 28 : 10)) {
        b.x = float(lx); --b.level; b.falling = true; b.jumped = false; break;
      }
    }
  }
  void tick(int sx,int sy,bool jump) {
    ++frames; if (immune) --immune; if (hammer) --hammer;
    if (--timeLeft <= 0) { loseLife(); return; }
    int horizontal = sx > 650 ? 1 : sx < -650 ? -1 : 0;
    if (horizontal) facing = horizontal;
    if (climbTo >= 0) {
      // Stop on the ladder when centered; permit reversing midway.
      if (sy > 650) y -= 0.55f;
      else if (sy < -650) y += 0.55f;
      if (y <= floorY(level+1,x)) { ++level; climbTo = -1; y = floorY(level,x); }
      else if (y >= floorY(level,x)) { climbTo = -1; y = floorY(level,x); }
    } else {
      if (!jumping) {
        if (jump && !hammer) { jumping = true; vy = -1.5f; }
        else if (!hammer && abs(sy) > 650) {
          int lower = sy > 0 ? level : level-1;
          if (lower >= 0 && lower < 4) for (int n = 0; n < 2; ++n) if (abs(x-ladderX(lower,n)) < 4) {
            x = float(ladderX(lower,n)); climbTo = lower+1; level = lower;
            y += sy > 0 ? -0.55f : 0.55f; break;
          }
        }
      }
      if (climbTo < 0) {
        x = constrain(x + horizontal*0.67f, 15.0f, 121.0f);
        if (jumping) {
          vy += 0.16f; y += vy;
          if (vy > 0 && y >= floorY(level,x)) { y = floorY(level,x); jumping = false; vy = 0; }
        } else y = floorY(level,x);
      }
    }
    if (!jumping && climbTo < 0 && !hammer) for (int i = 0; i < 2; ++i)
      if (hammers[i] && level == hammerLevel(i) && abs(x-hammerX(i)) < 4) { hammers[i] = false; hammer = 250; }
    if (--spawnTimer <= 0) spawn();
    for (auto &b : barrels) {
      roll(b); if (!b.active) continue;
      float bx = abs(x-b.x), by = abs((y-3)-(b.y-2));
      if (hammer && bx < 10 && by < 7) { b.active = false; score += 300; continue; }
      if (!immune && bx < 3.5f && by < 4) { loseLife(); return; }
      if (jumping && !b.falling && level == b.level && bx < 3 && y < b.y-5 && !b.jumped) {
        score += 100; b.jumped = true;
      }
    }
    fireX += fireDir * (hard ? 0.26f : 0.18f);
    if (fireX > 94) { fireX = 94; fireDir = -1; }
    if (fireX < 25) { fireX = 25; fireDir = 1; }
    if (level == 0 && abs(x-fireX) < 4 && abs(y-floorY(0,fireX)) < 5 && !immune) {
      if (hammer) { fireX = 27; fireDir = 1; score += 200; }
      else { loseLife(); return; }
    }
    if (level == 4 && climbTo < 0 && !jumping && x > 108) {
      score += 1000 + uint32_t(timeLeft/50)*10; score = min(score,uint32_t(999999)); cleared = 100;
    }
  }
  static void line(Adafruit_SSD1306 &d,int y,const char *s) { d.setCursor(0,y); d.print(s); }
  static void drawHammer(Adafruit_SSD1306 &d,int x,int y) {
    d.drawFastVLine(x,y,5,SSD1306_WHITE); d.fillRect(x-2,y,5,2,SSD1306_WHITE);
  }
  void gorilla(Adafruit_SSD1306 &d) const {
    int gy = int(floorY(4,13));
    d.fillRect(8,gy-6,9,5,SSD1306_WHITE); d.fillRect(10,gy-9,6,4,SSD1306_WHITE);
    d.drawPixel(11,gy-8,SSD1306_BLACK); d.drawPixel(14,gy-8,SSD1306_BLACK);
    d.drawFastHLine(11,gy-6,4,SSD1306_BLACK);
    int lift = spawnTimer < 20 ? 3 : 0;
    d.drawLine(8,gy-6,5,gy-1-lift,SSD1306_WHITE); d.drawLine(17,gy-6,21,gy-1-lift,SSD1306_WHITE);
    d.drawFastHLine(8,gy-1,4,SSD1306_WHITE); d.drawFastHLine(15,gy-1,4,SSD1306_WHITE);
  }
};
