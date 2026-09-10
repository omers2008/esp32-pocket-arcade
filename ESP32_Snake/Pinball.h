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
    d.drawRect(2, 1, 51, 62, SSD1306_WHITE);
    // Bumpers.
    drawBumper(d, 16, 23); drawBumper(d, 27, 14); drawBumper(d, 38, 23);
    // Two flippers: button-held positions lift toward the center.
    if (leftHeld) d.drawLine(10, 58, 25, 51, SSD1306_WHITE);
    else d.drawLine(10, 58, 25, 60, SSD1306_WHITE);
    if (rightHeld) d.drawLine(44, 58, 29, 51, SSD1306_WHITE);
    else d.drawLine(44, 58, 29, 60, SSD1306_WHITE);
    if (!waiting) d.fillCircle(int(ballX), int(ballY), 2, SSD1306_WHITE);
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
    if (ballX < 7) { ballX = 7; ballVx = abs(ballVx); }
    if (ballX > 47) { ballX = 47; ballVx = -abs(ballVx); }
    if (ballY < 5) { ballY = 5; ballVy = abs(ballVy); }
    hitBumper(16, 23); hitBumper(27, 14); hitBumper(38, 23);
    if (ballY > 51 && ballVy > 0) {
      if (leftHeld && ballX < 27 && ballX > 7) {
        ballY = 50; ballVy = -abs(ballVy) - 0.25f; ballVx += (ballX - 17) / 18.0f; score += 10;
      } else if (rightHeld && ballX > 27 && ballX < 47) {
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
