//#include "auth.hpp"
//#include "config.hpp"
#include <iostream>
#include <functional>
#include <vector>
#include <conio.h>
#include <stdexcept>

typedef unsigned int uint;

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

class menu {
	uint menu_x;
	uint menu_y;
	uint after_x;
	uint after_y;
	uint selected_item = 0;

	std::vector<std::string> labels;
	std::vector<std::function<void(void)>> callbacks;

public:
	menu(
		uint menu_x,
		uint menu_y,
		uint after_x,
		uint after_y,
		std::vector<std::string> labels,
		std::vector<std::function<void(void)>> callbacks
	) : menu_x(menu_x), 
		menu_y(menu_y), 
		after_x(after_x), 
		after_y(after_y), 
		labels(labels), 
		callbacks(callbacks)
	{
		if (labels.size() != callbacks.size()) {
			throw std::logic_error("labels.size() isn't equal to callbacks.size()");
		}
	}

	void move_up() {
		if (selected_item == 0) {
			selected_item = labels.size() - 1;
		}
		else {
			selected_item--;
		}
	}

	void move_down() {
		if (selected_item == labels.size() - 1) {
			selected_item = 0;
		}
		else {
			selected_item++;
		}
	}

	void draw() {
		console::move_to(menu_x, menu_y);
		for (int i = 0; i < labels.size(); i++) {
			console::move_to_column(menu_x);
			if (i == selected_item) {
				console::enable_inverse();
				std::cout << labels[i] << std::endl;
				console::disable_inverse();
			}
			else {
				std::cout << labels[i] << std::endl;
			}
		}

		console::move_to(after_x, after_y);
	}

	void execute() {
		callbacks[selected_item]();
	}
};

int main() {
	console::hide_cursor();

	uint log_x = 2;
	uint log_y = 8;

	menu* current_menu = nullptr;

	menu* submenu;

	menu* mainmenu = new menu(
		2, 2,
		log_x, log_y,
		{ "Exit Program", "Submenu", "Greet", "Third", "Fourth" },
		{ 
			[]() {
				exit(0);
			},
			[&current_menu, &submenu]() { 
				current_menu = submenu; 
				console::erase_screen(); 
			}, 
			[&log_x, &log_y]() { 
				console::show_cursor();
				std::string name;
				std::cout << "Enter your name: ";
				std::cin >> name;
				console::hide_cursor();
				console::move_to(log_x, log_y);
				console::erase_line();
				std::cout << "Hello, " << name << "!" << std::endl;
			}, 
			[]() { 
				printf("third"); 
			}, 
			[]() { 
				printf("fourth"); 
			} 
		}
	);

	submenu = new menu(
		2, 2,
		log_x, log_y,
		{ "Exit", "First", "Second" },
		{ [&current_menu, &mainmenu]() { current_menu = mainmenu; console::erase_screen(); }, []() { printf("subfirst"); }, []() { printf("subsecond"); } }
	);

	current_menu = mainmenu;

	current_menu->draw();

	for (;;) {
		unsigned char c, ex;

		c = getch();

		if (c && c != 224) {
			if (c == 10 || c == 13) {
				console::erase_line();
				console::move_to_column(log_x);
				current_menu->execute();
				current_menu->draw();
			}
		} else {
			switch (ex = getch()) {
			case 72:
				current_menu->move_up();
				current_menu->draw();
				break;
			case 80:
				current_menu->move_down();
				current_menu->draw();
				break;
			}
		}
	}
}