#pragma once
#include <Arduino.h>

// Normally-open button between GPIO and GND; one event per debounced press.
class ArcadeButton {
 public:
  explicit ArcadeButton(uint8_t pinNumber) : pin(pinNumber) {}

  void begin() {
    pinMode(pin, INPUT_PULLUP);
    raw = stable = digitalRead(pin);
    changedAt = millis();
  }

  void update() {
    pressed = false;
    bool reading = digitalRead(pin);
    uint32_t now = millis();
    if (reading != raw) {
      raw = reading;
      changedAt = now;
    }
    if (now - changedAt >= 25 && stable != raw) {
      stable = raw;
      pressed = stable == LOW;
    }
  }

  bool held() const { return stable == LOW; }
  bool released() const { return stable == HIGH && raw == HIGH; }
  bool pressed = false;

 private:
  uint8_t pin;
  bool raw = HIGH;
  bool stable = HIGH;
  uint32_t changedAt = 0;
};
