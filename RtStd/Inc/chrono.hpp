#pragma once

#include <chrono>
#include <cmsis_os2.h>

namespace rstd {
namespace chrono {
    class system_clock {
    public:
        using duration = std::chrono::duration<uint32_t, std::milli>;
        using rep = duration::rep;
        using period = duration::period;
        using time_point = std::chrono::time_point<system_clock>;

        static const bool is_steady = true; // constexpr in C++14

        static time_point now() noexcept { return time_point(duration(osKernelGetTickCount())); }

        static time_t to_time_t(const time_point &t) noexcept { return t.time_since_epoch().count(); }

        static time_point from_time_t(time_t t) noexcept { return time_point(duration(t)); }
    };

    using steady_clock = system_clock;

    using high_resolution_clock = system_clock;

    using nanoseconds = std::chrono::duration<uint32_t, std::nano>;

    using microseconds = std::chrono::duration<uint32_t, std::micro>;

    using milliseconds = std::chrono::duration<uint32_t, std::milli>;

    using seconds = std::chrono::duration<uint32_t>;

    using minutes = std::chrono::duration<uint32_t, std::ratio<60>>;

    using hours = std::chrono::duration<uint32_t, std::ratio<3600>>;
}
}
