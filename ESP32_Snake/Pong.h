#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

class Pong {
 public:
  uint8_t playerScore = 0, cpuScore = 0;
  uint32_t bestRally = 0;
  bool over = false;

  void start(bool hardMode) {
    hard = hardMode;
    playerScore = cpuScore = 0;
    bestRally = 0;
    over = false;
    playerY = cpuY = 36.0f;
    lastFrame = lastAim = millis();
    aimError = 0;
    cpuTarget = 36.0f;
    readyToServe();
  }

  void update(int stickY, bool servePressed) {
    if (over) return;
    // Keep a short button event even when it arrives between physics frames.
    if (waiting && servePressed) serveQueued = true;
    uint32_t now = millis();
    if (now - lastFrame < 20) return;
    lastFrame = now;

    if (stickY > 650) playerY -= 2.4f;
    if (stickY < -650) playerY += 2.4f;
    playerY = constrain(playerY, 17.0f, 56.0f);

    if (waiting) {
      if (serveQueued) {
        waiting = serveQueued = false;
        speed = hard ? 2.3f : 1.6f;
        vx = -speed; // Every new rally starts toward the player.
        vy = random(2) == 0 ? -0.65f : 0.65f;
      }
      return;
    }

    // Hard predicts wall bounces, but waits until the ball approaches midfield.
    // Both react every 100 ms. Hard predicts, but has more aiming error;
    // Easy tracks the current ball more accurately with a modest speed boost.
    if (now - lastAim >= 100u) {
      lastAim = now;
      aimError = hard ? random(-4, 5) : random(-2, 3);
      if (hard) {
        cpuTarget = vx > 0 && ballX > 54.0f ? predictedLanding() + aimError : 36.0f;
      } else {
        cpuTarget = vx > 0 ? ballY + aimError : 36.0f;
      }
    }
    float cpuSpeed = hard ? 2.0f : 1.65f;
    cpuY += constrain(cpuTarget - cpuY, -cpuSpeed, cpuSpeed);
    cpuY = constrain(cpuY, 17.0f, 56.0f);

    float oldX = ballX;
    ballX += vx;
    ballY += vy;
    if (ballY < 11.0f) { ballY = 22.0f - ballY; vy = -vy; }
    if (ballY > 62.0f) { ballY = 124.0f - ballY; vy = -vy; }

    if (vx < 0 && oldX >= 8.0f && ballX <= 8.0f && abs(ballY - playerY) <= 8.0f) {
      ballX = 8.0f;
      bounce(playerY, 1);
      ++rally;
      bestRally = max(bestRally, rally);
    } else if (vx > 0 && oldX <= 120.0f && ballX >= 120.0f && abs(ballY - cpuY) <= 8.0f) {
      ballX = 120.0f;
      bounce(cpuY, -1);
    }

    if (ballX < -2.0f || ballX > 129.0f) {
      if (ballX < 0) ++cpuScore;
      else ++playerScore;
      if (playerScore >= 7 || cpuScore >= 7) over = true;
      else readyToServe();
    }
  }

  void draw(Adafruit_SSD1306 &d) {
    d.clearDisplay();
    d.setTextColor(SSD1306_WHITE);
    d.setTextSize(1);
    d.setCursor(0, 0); d.print(F("YOU ")); d.print(playerScore);
    d.setCursor(61, 0); d.print(hard ? F("H") : F("E"));
    d.setCursor(94, 0); d.print(F("CPU ")); d.print(cpuScore);
    d.drawFastHLine(0, 9, 128, SSD1306_WHITE);
    for (int y = 12; y < 64; y += 6) d.drawFastVLine(64, y, 3, SSD1306_WHITE);
    d.fillRect(4, int(playerY) - 7, 3, 14, SSD1306_WHITE);
    d.fillRect(122, int(cpuY) - 7, 3, 14, SSD1306_WHITE);
    if (waiting) {
      d.fillRect(24, 27, 81, 20, SSD1306_BLACK);
      d.setCursor(37, 28); d.print(F("13: SERVE"));
      d.setCursor(34, 40); d.print(F("First to 7"));
    } else {
      d.fillRect(int(ballX) - 1, int(ballY) - 1, 3, 3, SSD1306_WHITE);
    }
    d.display();
  }

 private:
  float playerY = 36, cpuY = 36, ballX = 64, ballY = 36;
  float vx = 0, vy = 0, speed = 1.6f, aimError = 0;
  float cpuTarget = 36.0f;
  bool hard = false;
  uint32_t lastFrame = 0, lastAim = 0, rally = 0;
  bool waiting = true, serveQueued = false;

  void readyToServe() {
    ballX = 64; ballY = 36;
    vx = vy = 0;
    rally = 0;
    waiting = true;
    serveQueued = false;
  }

  void bounce(float paddleY, int horizontalDirection) {
    speed = min(speed + (hard ? 0.16f : 0.12f), hard ? 3.8f : 3.1f);
    vx = horizontalDirection * speed;
    // Paddle edges send the ball away at a steeper angle.
    vy = constrain((ballY - paddleY) / 7.0f, -1.0f, 1.0f) * (hard ? 2.6f : 2.1f);
    if (abs(vy) < 0.35f) vy = vy < 0 ? -0.35f : 0.35f;
  }

  float predictedLanding() const {
    float landing = ballY + vy * max(0.0f, (120.0f - ballX) / vx);
    while (landing < 11.0f || landing > 62.0f) {
      if (landing < 11.0f) landing = 22.0f - landing;
      if (landing > 62.0f) landing = 124.0f - landing;
    }
    return landing;
  }
};
