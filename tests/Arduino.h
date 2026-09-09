#pragma once
#include <cstdint>
#include <cstdlib>
#include <algorithm>
using std::min;
using std::max;
extern uint32_t testClock;
inline uint32_t millis() { return testClock; }
inline long random(long limit) { return std::rand() % limit; }
#define F(value) value
