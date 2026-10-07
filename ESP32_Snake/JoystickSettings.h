#pragma once

struct JoystickSettings {
  static constexpr uint16_t DEFAULT_DEADZONE = 650;
  static constexpr uint16_t MIN_DEADZONE = 100;
  static constexpr uint16_t MAX_DEADZONE = 1400;
  static constexpr uint16_t DEADZONE_STEP = 50;
  static constexpr uint8_t DEFAULT_SENSITIVITY = 100;
  static constexpr uint8_t MIN_SENSITIVITY = 50;
  static constexpr uint8_t MAX_SENSITIVITY = 200;
  static constexpr uint8_t SENSITIVITY_STEP = 10;

  uint16_t deadzoneX = DEFAULT_DEADZONE;
  uint16_t deadzoneY = DEFAULT_DEADZONE;
  uint8_t sensitivity = DEFAULT_SENSITIVITY;
  bool linked = false;

  void reset() {
    deadzoneX = DEFAULT_DEADZONE;
    deadzoneY = DEFAULT_DEADZONE;
    sensitivity = DEFAULT_SENSITIVITY;
    linked = false;
  }

  void setLinked(bool value) {
    linked = value;
    if (linked) deadzoneY = deadzoneX;
  }

  bool adjustDeadzoneX(int direction) {
    return setDeadzone(deadzoneX, direction);
  }

  bool adjustDeadzoneY(int direction) {
    if (linked) return adjustDeadzoneX(direction);
    return setDeadzone(deadzoneY, direction);
  }

  bool adjustSensitivity(int direction) {
    int value = int(sensitivity) + direction * SENSITIVITY_STEP;
    if (value < MIN_SENSITIVITY || value > MAX_SENSITIVITY) return false;
    sensitivity = value;
    return true;
  }

  int applyX(int value) const { return apply(value, deadzoneX); }
  int applyY(int value) const { return apply(value, deadzoneY); }

 private:
  bool setDeadzone(uint16_t &value, int direction) {
    int next = int(value) + direction * DEADZONE_STEP;
    if (next < MIN_DEADZONE || next > MAX_DEADZONE) return false;
    value = next;
    if (linked) deadzoneY = deadzoneX;
    return true;
  }

  int apply(int value, uint16_t deadzone) const {
    if (value >= -int(deadzone) && value <= int(deadzone)) return 0;
    long scaled = long(value) * sensitivity / 100;
    if (scaled > 2047) scaled = 2047;
    if (scaled < -2048) scaled = -2048;
    return int(scaled);
  }
};
