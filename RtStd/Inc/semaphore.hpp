#pragma once

#include <chrono>
#include <cmsis_os2.h>

namespace rstd {
    class _semaphore_base {
    public:
        using native_handle_type = osSemaphoreId_t;

        explicit _semaphore_base(uint32_t max, uint32_t now) noexcept: handle(osSemaphoreNew(max, now, nullptr)) {}

        _semaphore_base(const _semaphore_base &) = delete;

        _semaphore_base &operator=(const _semaphore_base &) = delete;

        ~_semaphore_base() { osSemaphoreDelete(handle); }

        void acquire() { osSemaphoreAcquire(handle, osWaitForever); }

        bool try_acquire() { return osSemaphoreAcquire(handle, 0) == osOK; }

        void release() { osSemaphoreRelease(handle); }

        native_handle_type native_handle() { return handle; }
    private:
        native_handle_type handle{};
    };

    class _semaphore_timed_base : public _semaphore_base {
    public:
        template<class Rep, class Period>
        bool try_acquire_for(const std::chrono::duration<Rep, Period> &rel_time) {
            return osSemaphoreAcquire(
                    native_handle(),
                    std::chrono::duration_cast<std::chrono::milliseconds>(rel_time).count()
            ) == osOK;
        }

        template<class Clock, class Duration>
        bool try_acquire_until(const std::chrono::time_point<Clock, Duration> &abs_time) {
            return try_acquire_for(abs_time - Clock::now());
        }

    protected:
        using _semaphore_base::_semaphore_base;
    };

    struct semaphore : _semaphore_base {
        using _semaphore_base::_semaphore_base;
    };

    struct timed_semaphore : _semaphore_timed_base {
        using _semaphore_timed_base::_semaphore_timed_base;
    };
}
