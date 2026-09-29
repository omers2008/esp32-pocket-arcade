#pragma once
#include <cstdint>
struct SPIClass {
  int clock = -1, input = -1, output = -1, chip = -1;
  void begin(int sck, int miso, int mosi, int cs) { clock=sck; input=miso; output=mosi; chip=cs; }
};
inline SPIClass SPI;
