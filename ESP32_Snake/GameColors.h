#pragma once
#include <stdint.h>

// RGB565 colors. Values 0, 1 and 2 remain reserved for the legacy canvas API.
namespace Ink {
constexpr uint16_t White=0xF7BE, Muted=0x8CB3, Wall=0x4350;
constexpr uint16_t Blue=0x3B7F, Cyan=0x2E9F, Green=0x5F54;
constexpr uint16_t Red=0xF9C7, Pink=0xFC19, Purple=0xB2FF;
constexpr uint16_t Gold=0xFFE6, Orange=0xFD28, Brown=0xA346;
constexpr uint16_t Skin=0xFE15, Dark=0x0863;
inline uint16_t ghost(int index) {
  const uint16_t colors[]={Red,Pink,Cyan,Orange};
  return colors[index%4];
}
}
