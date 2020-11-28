#include <console.hpp>
#include <cstring>

namespace {
    char* gPool = nullptr, *gLoc {};
    int gRoll = 0, gWidth{}, gHeight{};
    rstd::console::display_handler_t gHandler = nullptr;
}

namespace rstd {
    void console::init(int width, int height) {
    	const auto size = height * (width + 1);
    	std::memset(gLoc = gPool = new char[size], 0, size);
    	gWidth = width + 1, gHeight = height;
    }

    void console::set_display_handler(console::display_handler_t handler) noexcept {
    	gHandler = handler;
    }

    console::display_handler_t console::get_display_handler() noexcept {
    	return gHandler;
    }

    void console::flush() noexcept {
    	for (int i = 0; i < gHeight; ++i) {
    		const auto line = (i + gRoll) % gHeight;
    		const auto base = gPool + gWidth * line;
    		gHandler(base, i);
    	}
    }
}

extern "C" int __io_putchar(int ch) noexcept {
	if (!gLoc) goto fn_exit;
	if (ch!='\n') {
		*gLoc = static_cast<char>(ch);
		if ((++gLoc - gPool) % gWidth == (gWidth - 1)) goto relocate;
		goto fn_exit;
	}
	else {
		do {
		*gLoc = ' ';
		} while (((++gLoc - gPool) % gWidth != (gWidth - 1)));
	}
	relocate:
	gRoll = (gRoll + 1) % gHeight;
	gLoc = gPool + gWidth * gRoll;
	fn_exit: return 0;
}
