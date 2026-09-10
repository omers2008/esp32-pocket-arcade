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
    d.setCursor(0, 0); d.print(F("PINBALL  S")); d.print(score);
    d.setCursor(92, 0); d.print(F("L")); d.print(lives);
    d.drawFastHLine(0, 8, 128, SSD1306_WHITE);
    d.drawRect(4, 10, 120, 53, SSD1306_WHITE);
    // Bumpers.
    drawBumper(d, 35, 25); drawBumper(d, 64, 18); drawBumper(d, 93, 25);
    // Two flippers: button-held positions lift toward the center.
    if (leftHeld) d.drawLine(18, 57, 47, 48, SSD1306_WHITE);
    else d.drawLine(18, 57, 48, 56, SSD1306_WHITE);
    if (rightHeld) d.drawLine(110, 57, 81, 48, SSD1306_WHITE);
    else d.drawLine(110, 57, 80, 56, SSD1306_WHITE);
    if (!waiting) d.fillCircle(int(ballX), int(ballY), 2, SSD1306_WHITE);
    else {
      d.drawCircle(64, 48, 3, SSD1306_WHITE);
      d.setCursor(29, 36); d.print(F("13/14 START"));
    }
    d.display();
  }

 private:
  float ballX = 64, ballY = 48, ballVx = 0, ballVy = 0;
  bool hard = false, waiting = true, leftHeld = false, rightHeld = false, launchReady = false;
  uint32_t lastFrame = 0;

  void resetBall() {
    ballX = 64; ballY = 48; ballVx = ballVy = 0;
  }

  static void drawBumper(Adafruit_SSD1306 &d, int x, int y) {
    d.drawCircle(x, y, 6, SSD1306_WHITE);
    d.fillCircle(x, y, 2, SSD1306_WHITE);
  }

  void hitBumper(int x, int y) {
    float dx = ballX - x, dy = ballY - y;
    if (dx * dx + dy * dy >= 64.0f) return;
    ballVy = -abs(ballVy) - 0.15f;
    ballVx += dx < 0 ? -0.35f : 0.35f;
    ballX = x + (dx < 0 ? -9 : 9);
    score += 25;
  }

  void stepBall() {
    ballVy += hard ? 0.075f : 0.06f;
    ballX += ballVx; ballY += ballVy;
    if (ballX < 8) { ballX = 8; ballVx = abs(ballVx); }
    if (ballX > 120) { ballX = 120; ballVx = -abs(ballVx); }
    if (ballY < 13) { ballY = 13; ballVy = abs(ballVy); }
    hitBumper(35, 25); hitBumper(64, 18); hitBumper(93, 25);
    if (ballY > 51 && ballVy > 0) {
      if (leftHeld && ballX < 57 && ballX > 12) {
        ballY = 50; ballVy = -abs(ballVy) - 0.25f; ballVx += (ballX - 32) / 24.0f; score += 10;
      } else if (rightHeld && ballX > 71 && ballX < 116) {
        ballY = 50; ballVy = -abs(ballVy) - 0.25f; ballVx += (ballX - 96) / 24.0f; score += 10;
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
