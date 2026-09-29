#pragma once
#include <stdint.h>

// GMT020-02-8P, ST7789 240x320, landscape. All signals use 3.3V logic.
// BL and VCC connect to 3.3V. No MISO connection is needed.
namespace ArcadeScreen {
constexpr int8_t SCLK = 18; // Screen SCL (SPI clock, NOT I2C SCL).
constexpr int8_t MOSI = 23; // Screen SDA (SPI data, NOT I2C SDA).
constexpr int8_t CS = 27;   // Disconnect any old joystick B wire here.
constexpr int8_t DC = 26;
constexpr int8_t RESET = 25;
constexpr uint8_t ROTATION = 1; // Change to 3 for the other landscape orientation.
constexpr bool INVERT = true; // ST7789 IPS panels generally require inversion.
constexpr uint32_t SPI_HZ = 20000000; // Conservative speed for jumper wires.
constexpr int WIDTH = 128, HEIGHT = 64, SCALE = 2;
constexpr uint16_t FOREGROUND = 0xFFFF, BACKGROUND = 0x0000;
}
