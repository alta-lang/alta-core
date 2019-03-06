#ifndef ALTACORE_TIMING_HPP
#define ALTACORE_TIMING_HPP

#include <chrono>
#include <vector>
#include "simple-map.hpp"
#include "fs.hpp"

namespace AltaCore {
  namespace Timing {
    const auto toSeconds = [](auto&& duration) {
      return std::chrono::duration_cast<std::chrono::seconds>(duration);
    };
    const auto toMilliseconds = [](auto&& duration) {
      return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    };
    const auto toMicroseconds = [](auto&& duration) {
      return std::chrono::duration_cast<std::chrono::microseconds>(duration);
    };

    class Timer {
      public:
        using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

      private:
        std::vector<std::pair<TimePoint, TimePoint>> times;
        bool stopped = true;

      public:
        Timer(bool startNow = false);

        void start();
        void stop();

        std::chrono::nanoseconds recent();
        std::chrono::nanoseconds total();
    };

    template<typename T> using TimeTable = ALTACORE_MAP<T, Timer>;
    using FileTimeTable = TimeTable<Filesystem::Path>;
    using StringTimeTable = TimeTable<std::string>;

    extern FileTimeTable preprocessTimes;
    extern FileTimeTable lexTimes;
    extern FileTimeTable parseTimes;
  };
};

#endif /* ALTACORE_TIMING_HPP */
