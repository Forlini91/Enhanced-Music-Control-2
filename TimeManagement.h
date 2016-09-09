#pragma once

#include <chrono>

using namespace std::chrono;

#define ONE_SECOND 10000000.0


extern const milliseconds TIME_ZERO;
extern milliseconds now;
extern long long timeStamp;


void updateNow ();