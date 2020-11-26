namespace rstd {
    struct console {
    	using display_handler_t = void(*)(const char* text, int line) noexcept;
    	static void init(int width, int height);
    	static void set_display_handler(display_handler_t handler) noexcept;
    	static display_handler_t get_display_handler() noexcept;
    	static void flush() noexcept;
    };
}
