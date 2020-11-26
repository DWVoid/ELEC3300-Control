#include <thread.hpp>
#include <console.hpp>
#include <typeinfo>

void rstd::thread::Crash(std::exception& x) {
	auto thread_id = rstd::this_thread::get_id();
	auto what = x.what();
	std::puts("Thread crash for uncaught exception:");
	std::puts("With Type:");
	std::puts(typeid(x).name());
	std::puts("With reason:");
	std::puts(what);
	std::printf("With handle: %#010x\n", reinterpret_cast<uintptr_t>(thread_id.native()));
	rstd::console::flush();
	for (;;) {} // Hang up in infinite loop
}
