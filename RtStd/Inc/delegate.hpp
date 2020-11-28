#include <memory>

namespace rstd {
void _delegate_run(void(*function)(void*) noexcept, void* user) noexcept;

void _delegate_spawn(void(*function)(void*) noexcept, void* user) noexcept;

    [[noreturn]] void hold_indefinitely() noexcept;

    template <class F>
    void run(F fn) {
       	_delegate_run(
       			[](void* x) noexcept { (*reinterpret_cast<F*>(x))(); },
   				reinterpret_cast<void*>(&fn)
   		);
    }

    template <class F>
    void async_spawn(F&& fn) {
    	auto unique = std::make_unique<F>(std::forward<F>(fn));
    	const auto bind = [](void* x) noexcept {
    		auto unique = std::unique_ptr<F>(reinterpret_cast<F*>(x));
    		(*unique)();
    	};
    	_delegate_spawn(
    			bind,
   				reinterpret_cast<void*>(unique.release())
    	);
    }
}
