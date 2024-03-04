#include <iostream>

enum class color {
	Black = 0,
	Red = 1,
	Green = 2,
	Yellow = 3,
	Blue = 4,
	Magenta = 5,
	Cyan = 6,
	White = 7,
	Default = 9
};

struct console {
	typedef std::function<void(void)> callback;
	typedef unsigned int uint;

	static void move_to(uint x, uint y) { printf("\x1b[%d;%dH", y, x); }
	static void move_to_column(uint x) { printf("\x1b[%dG", x); }
	static void move_to_origin() { printf("\x1b[H"); }

	static void erase_screen() { printf("\x1b[2J"); }
	static void erase_line() { printf("\x1b[2K"); }

	static void move_up(uint amount) { printf("\x1b[%dA", amount); }
	static void move_down(uint amount) { printf("\x1b[%dB", amount); }
	static void move_right(uint amount) { printf("\x1b[%dC", amount); }
	static void move_left(uint amount) { printf("\x1b[%dD", amount); }

	static void enable_bold() { printf("\x1b[1m"); }
	static void disable_bold() { printf("\x1b[22m"); }
	static void bold(callback c) { enable_bold(); c(); disable_bold(); }

	static void enable_italic() { printf("\x1b[3m"); }
	static void disable_italic() { printf("\x1b[23m"); }
	static void italic(callback c) { enable_italic(); c(); disable_italic(); }

	static void enable_underline() { printf("\x1b[4m"); }
	static void disable_underline() { printf("\x1b[24m"); }
	static void underline(callback c) { enable_underline(); c(); disable_underline(); }

	static void enable_inverse() { printf("\x1b[7m"); }
	static void disable_inverse() { printf("\x1b[27m"); }
	static void inverse(callback c) { enable_inverse(); c(); disable_inverse(); }

	static void enable_strikethrough() { printf("\x1b[9m"); }
	static void disable_strikethrough() { printf("\x1b[29m"); }
	static void strikethrough(callback c) { enable_strikethrough(); c(); disable_strikethrough(); }

	static void set_color(color c, bool foreground) {
		uint foreground_flag = 3;
		if (!foreground) {
			foreground_flag = 4;
		}

		uint color_index = static_cast<uint>(c);

		printf("\x1b[%d%dm", foreground_flag, color_index);
	}
	static void set_foreground_color(color c) {
		set_color(c, true);
	}
	static void set_background_color(color c) {
		set_color(c, false);
	}

	static void reset_color(bool foreground) {
		set_color(color::Default, foreground);
	}
	static void reset_foreground_color() {
		reset_color(true);
	}
	static void reset_background_color() {
		reset_color(false);
	}

	static void hide_cursor() {
		printf("\x1b[?25l");
	}
	static void show_cursor() {
		printf("\x1b[?25h");
	}
};