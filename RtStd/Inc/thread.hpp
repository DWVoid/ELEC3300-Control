#pragma once

#include <future>
#include <chrono>
#include <type_traits>
#include <cmsis_os2.h>

namespace rstd {
    template<class Callable, class ...Ts>
    class _defer_callable {
        template<std::size_t... I>
        auto apply_impl(std::index_sequence<I...>) { return fn(std::move(std::get<I>(args))...); }

    protected:
        using return_type = std::result_of_t<std::decay_t<Callable>(std::decay_t<Ts>...)>;
    public:
        explicit _defer_callable(Callable &&call, Ts &&... args)
                : fn(std::forward<Callable>(call)), args(std::forward<Ts>(args)...) {}

        auto call() { return apply_impl(std::make_index_sequence<std::tuple_size<decltype(args)>::value>()); }

    private:
        Callable fn;
        std::tuple<Ts...> args;
    };

    template<class Callable>
    class _defer_callable<Callable> {
    protected:
        using return_type = std::result_of_t<std::decay_t<Callable>()>;
    public:
        explicit _defer_callable(Callable &&call)
                : fn(std::forward<Callable>(call)) {}

        auto call() { return fn(); }

    private:
        Callable fn;
    };

    class thread {
    	[[noreturn]] static void Crash(std::exception& x);
    public:
        using native_handle_type = osThreadId_t;

        class id {
        public:
            id() noexcept = default;

            explicit id(native_handle_type h) noexcept: handle(h) {}

            bool operator==(thread::id y) const noexcept { return handle == y.handle; }

            bool operator!=(thread::id y) const noexcept { return handle != y.handle; }

            bool operator<(thread::id y) const noexcept { return handle < y.handle; }

            bool operator<=(thread::id y) const noexcept { return handle <= y.handle; }

            bool operator>(thread::id y) const noexcept { return handle > y.handle; }

            bool operator>=(thread::id y) const noexcept { return handle >= y.handle; }

            native_handle_type native() const noexcept { return handle; }

        private:
            native_handle_type handle{ nullptr };
        };

        thread() noexcept: handle(nullptr) {}

        template<class F, class ...Args>
        explicit thread(osPriority_t priority, uint32_t stackSize, F &&f, Args &&... args) {
            osThreadAttr_t attr = {
                    .name = nullptr,
                    .attr_bits = osThreadDetached,
                    .cb_mem = nullptr,
                    .cb_size = 0,
                    .stack_mem = nullptr,
                    .stack_size = stackSize,
                    .priority = priority,
                    .tz_module = 0,
                    .reserved = 0
            };
            using T = _defer_callable<F, Args...>;
            if (sizeof(T) <= sizeof(void *) && std::is_trivially_copyable<T>::value) {
                union A {
                    T obj;
                    void *spacing;

                    explicit A(F &&f, Args &&... args): obj(std::forward<F>(f), std::forward<Args>(args)...) {}
                };
                handle = osThreadNew([](void *arg) {
                	try {
                        reinterpret_cast<A*>(&arg)->obj.call();
                        osThreadExit();
                	}
                	catch (std::exception& x) { Crash(x); }
                }, A(std::forward<F>(f)).spacing, &attr);
            } else {
                const auto set = new T(std::forward<F>(f), std::forward<Args>(args)...);
                handle = osThreadNew([](void *arg) {
                	try {
                        reinterpret_cast<T*>(arg)->call();
                        delete reinterpret_cast<T*>(arg);
                        osThreadExit();
                	}
                	catch (std::exception& x) { Crash(x); }
                }, set, &attr);
            }
        }

        ~thread() = default;

        thread(const thread &) = delete;

        thread(thread &&t) noexcept: handle(t.handle) { t.handle = nullptr; }

        thread &operator=(const thread &) = delete;

        thread &operator=(thread &&t) noexcept {
            if (this != &t) {
                this->handle = t.handle;
                t.handle = nullptr;
            }
            return *this;
        }

        void swap(thread &t) noexcept {
            const auto x = t.handle;
            t.handle = handle;
            handle = x;
        }

        [[nodiscard]] bool joinable() const noexcept { return handle != nullptr; }

        void join() {
            while (osThreadGetState(handle) != osThreadTerminated) osDelay(1);
            handle = nullptr;
        }

        void detach() { handle = nullptr; }

        [[nodiscard]] id get_id() const noexcept { return id(handle); }

        native_handle_type native_handle() { return handle; }

        static unsigned hardware_concurrency() noexcept { return 1; } // Assume STM32F103
    private:
        native_handle_type handle;
    };

    inline void swap(thread &x, thread &y) noexcept { x.swap(y); }

    namespace this_thread {
        inline thread::id get_id() noexcept { return thread::id(osThreadGetId()); }

        inline void yield() noexcept { osThreadYield(); }

        template<class Clock, class Duration>
        inline void sleep_until(const std::chrono::time_point<Clock, Duration> &abs_time) {
            osDelay(std::chrono::duration_cast<std::chrono::duration<uint32_t, std::milli>>(abs_time.time_since_epoch()).count());
        }

        template<class Rep, class Period>
        inline void sleep_for(const std::chrono::duration<Rep, Period> &rel_time) {
            osDelay(std::chrono::duration_cast<std::chrono::duration<uint32_t, std::milli>>(rel_time).count());
        }
    }
}
