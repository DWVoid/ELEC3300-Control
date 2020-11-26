#pragma once

#include <mutex>
#include <chrono>
#include <cmsis_os2.h>

namespace rstd {
    class _mutex_base {
    public:
        using native_handle_type = osMutexId_t;

        _mutex_base(const _mutex_base &) = delete;

        _mutex_base &operator=(const _mutex_base &) = delete;

        ~_mutex_base() { osMutexDelete(handle); }

        void lock() { osMutexAcquire(handle, osWaitForever); }

        bool try_lock() { return osMutexAcquire(handle, 0) == osOK; }

        void unlock() { osMutexRelease(handle); }

        native_handle_type native_handle() { return handle; }

    protected:
        explicit _mutex_base(const osMutexAttr_t &attr) noexcept: handle(osMutexNew(&attr)) {}

        static osMutexAttr_t &GetConfigA() noexcept;

        static osMutexAttr_t &GetConfigB() noexcept;

    private:
        native_handle_type handle{};
    };

    class _mutex_timed_base : public _mutex_base {
    public:
        template<class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep, Period> &rel_time) {
            return osMutexAcquire(
                    native_handle(),
                    std::chrono::duration_cast<std::chrono::milliseconds>(rel_time).count()
            ) == osOK;
        }

        template<class Clock, class Duration>
        bool try_lock_until(const std::chrono::time_point<Clock, Duration> &abs_time) {
            return try_lock_for(Clock::now() + abs_time);
        }

    protected:
        using _mutex_base::_mutex_base;
    };

    struct mutex : _mutex_base {
        mutex() noexcept: _mutex_base(GetConfigA()) {}
    };

    struct recursive_mutex : _mutex_base {
        recursive_mutex() noexcept: _mutex_base(GetConfigB()) {}
    };

    struct timed_mutex : _mutex_timed_base {
        timed_mutex() noexcept: _mutex_timed_base(GetConfigA()) {}
    };

    struct timed_recursive_mutex : _mutex_timed_base {
        timed_recursive_mutex() noexcept: _mutex_timed_base(GetConfigB()) {}
    };
}