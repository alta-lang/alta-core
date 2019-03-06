#include "../include/altacore/timing.hpp"

namespace AltaCore {
  namespace Timing {
    FileTimeTable preprocessTimes;
    FileTimeTable lexTimes;
    FileTimeTable parseTimes;
  };
};

AltaCore::Timing::Timer::Timer(bool startNow) {
  if (startNow) {
    start();
  }
};


void AltaCore::Timing::Timer::start() {
  if (!stopped) {
    stop();
  }

  times.emplace_back(std::chrono::high_resolution_clock::now(), std::chrono::time_point<std::chrono::high_resolution_clock>::min());

  stopped = false;
};

void AltaCore::Timing::Timer::stop() {
  if (times.size() > 0) {
    times.back().second = std::chrono::high_resolution_clock::now();
  }

  stopped = true;
};

std::chrono::nanoseconds AltaCore::Timing::Timer::recent() {
  if (times.size() < 1) {
    return std::chrono::nanoseconds::min();
  }

  return times.back().second - times.back().first;
};

std::chrono::nanoseconds AltaCore::Timing::Timer::total() {
  if (times.size() < 1) {
    return std::chrono::nanoseconds::min();
  }

  auto total = times.front().second - times.front().first;

  for (auto it = times.begin() + 1; it != times.end(); it++) {
    total += it->second - it->first;
  }

  return total;
};
