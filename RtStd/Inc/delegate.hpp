namespace rstd {
    void _delegate_run(void(*function)(void*) noexcept, void* user) noexcept;

    [[noreturn]] void hold_indefinitely() noexcept;

    template <class F>
    void run(F fn) {
       	_delegate_run(
       			[](void* x) noexcept { (*reinterpret_cast<F*>(x))(); },
   				reinterpret_cast<void*>(&fn)
   		);
    }
}
