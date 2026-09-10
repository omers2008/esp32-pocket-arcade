#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// A compact original pinball table with two button-controlled flippers.
class Pinball {
 public:
  uint32_t score = 0;
  int lives = 3;
  bool over = false, won = false;

  void start(bool hardMode) {
    hard = hardMode; score = 0; lives = hard ? 2 : 3; over = won = false;
    leftHeld = rightHeld = false; launchReady = false;
    waiting = true; resetBall();
    lastFrame = millis();
  }

  void update(bool leftButtonHeld, bool rightButtonHeld) {
    if (over) return;
    leftHeld = leftButtonHeld; rightHeld = rightButtonHeld;
    uint32_t now = millis();
    if (!leftHeld && !rightHeld) launchReady = true;
    if (waiting) {
      if (launchReady && (leftHeld || rightHeld)) {
        waiting = false; launchReady = false;
        ballVx = rightHeld && !leftHeld ? 0.85f : -0.85f;
        ballVy = -(hard ? 2.15f : 1.85f);
      }
      return;
    }
    if (now - lastFrame < 20) return;
    lastFrame = now;
    stepBall();
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay(); d.setTextSize(1); d.setTextColor(SSD1306_WHITE);
    // A narrow, tall table leaves the right side for readable status information,
    // like the compact Tetris layout.
    d.drawFastVLine(56, 0, 64, SSD1306_WHITE);
    d.drawRect(1, 1, 54, 62, SSD1306_WHITE);
    // Small, widely spaced bumpers, like a real tabletop's upper playfield.
    drawBumper(d, 13, 15); drawBumper(d, 27, 9); drawBumper(d, 41, 15);
    drawBumper(d, 18, 25); drawBumper(d, 36, 25);
    // Narrow posts and three drop targets add different obstacle shapes.
    drawPost(d, 10, 31, 39); drawPost(d, 17, 33, 42);
    drawPost(d, 37, 33, 42); drawPost(d, 44, 31, 39);
    drawTarget(d, 22, 32); drawTarget(d, 27, 32); drawTarget(d, 32, 32);
    // Angled guides funnel the ball toward the flippers.
    d.drawLine(7, 45, 15, 49, SSD1306_WHITE);
    d.drawLine(47, 45, 39, 49, SSD1306_WHITE);
    // Two flippers: button-held positions lift toward the center.
    if (leftHeld) d.drawLine(9, 58, 23, 52, SSD1306_WHITE);
    else d.drawLine(9, 58, 22, 60, SSD1306_WHITE);
    if (rightHeld) d.drawLine(45, 58, 31, 52, SSD1306_WHITE);
    else d.drawLine(45, 58, 32, 60, SSD1306_WHITE);
    if (!waiting) d.fillCircle(int(ballX), int(ballY), 1, SSD1306_WHITE);
    else {
      d.drawCircle(27, 47, 3, SSD1306_WHITE);
      d.setCursor(15, 38); d.print(F("START"));
    }
    d.setCursor(60, 2); d.print(F("PINBALL"));
    d.setCursor(60, 12); d.print(F("SCORE"));
    d.setCursor(60, 21); d.print(score);
    d.setCursor(60, 31); d.print(F("BALLS"));
    d.setCursor(60, 40); d.print(lives);
    d.setCursor(60, 49); d.print(hard ? F("HARD") : F("EASY"));
    d.setCursor(60, 58); d.print(F("13=L 14=R"));
    d.display();
  }

 private:
  float ballX = 64, ballY = 48, ballVx = 0, ballVy = 0;
  bool hard = false, waiting = true, leftHeld = false, rightHeld = false, launchReady = false;
  uint32_t lastFrame = 0;

  void resetBall() {
    ballX = 27; ballY = 47; ballVx = ballVy = 0;
  }

  static void drawBumper(Adafruit_SSD1306 &d, int x, int y) {
    d.drawCircle(x, y, 2, SSD1306_WHITE);
    d.fillCircle(x, y, 1, SSD1306_WHITE);
  }

  static void drawPost(Adafruit_SSD1306 &d, int x, int y1, int y2) {
    d.drawFastVLine(x, y1, y2 - y1 + 1, SSD1306_WHITE);
    d.drawPixel(x, y1 - 1, SSD1306_WHITE);
    d.drawPixel(x, y2 + 1, SSD1306_WHITE);
  }

  static void drawTarget(Adafruit_SSD1306 &d, int x, int y) {
    d.drawRect(x, y, 3, 2, SSD1306_WHITE);
    d.drawPixel(x + 1, y, SSD1306_WHITE);
  }

  void hitBumper(int x, int y) {
    float dx = ballX - x, dy = ballY - y;
    if (dx * dx + dy * dy >= 16.0f) return;
    ballVy = -abs(ballVy) - 0.15f;
    ballVx += dx < 0 ? -0.35f : 0.35f;
    ballX = x + (dx < 0 ? -4 : 4);
    score += 25;
  }

  void hitPost(int x, int y1, int y2) {
    if (ballY < y1 - 2 || ballY > y2 + 2 || ballX < x - 2 || ballX > x + 2) return;
    ballVx = ballX < x ? -abs(ballVx) - 0.25f : abs(ballVx) + 0.25f;
    ballX = x + (ballX < x ? -3 : 3);
    score += 5;
  }

  void hitTarget(int x, int y) {
    if (ballX < x - 2 || ballX > x + 5 || ballY < y - 2 || ballY > y + 4) return;
    ballVy = ballY < y ? -abs(ballVy) - 0.2f : abs(ballVy) + 0.2f;
    ballY = y + (ballY < y ? -3 : 5);
    score += 15;
  }

  void stepBall() {
    ballVy += hard ? 0.075f : 0.06f;
    ballX += ballVx; ballY += ballVy;
    if (ballX < 5) { ballX = 5; ballVx = abs(ballVx); }
    if (ballX > 49) { ballX = 49; ballVx = -abs(ballVx); }
    if (ballY < 5) { ballY = 5; ballVy = abs(ballVy); }
    hitBumper(13, 15); hitBumper(27, 9); hitBumper(41, 15);
    hitBumper(18, 25); hitBumper(36, 25);
    hitPost(9, 31, 37); hitPost(16, 33, 40);
    hitPost(38, 33, 40); hitPost(45, 31, 37);
    hitTarget(22, 32); hitTarget(27, 32); hitTarget(32, 32);
    if (ballY > 51 && ballVy > 0) {
      if (leftHeld && ballX < 27 && ballX > 5) {
        ballY = 50; ballVy = -abs(ballVy) - 0.25f; ballVx += (ballX - 17) / 18.0f; score += 10;
      } else if (rightHeld && ballX > 27 && ballX < 49) {
        ballY = 50; ballVy = -abs(ballVy) - 0.25f; ballVx += (ballX - 37) / 18.0f; score += 10;
      }
    }
    ballVx = constrain(ballVx, -3.2f, 3.2f);
    if (ballY > 64) {
      --lives;
      if (!lives) { over = true; return; }
      waiting = true; launchReady = false; resetBall();
    }
  }
};
