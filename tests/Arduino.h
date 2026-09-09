#pragma once
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <cmath>
using std::abs;
using std::min;
using std::max;
extern uint32_t testClock;
inline uint32_t millis() { return testClock; }
inline long random(long limit) { return std::rand() % limit; }
inline long random(long lower, long upper) { return lower + random(upper - lower); }
template<class T> T constrain(T value, T lower, T upper) { return min(max(value, lower), upper); }
#define F(value) value
