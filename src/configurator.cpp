//#include "auth.hpp"
//#include "config.hpp"
#include <iostream>
#include <vector>
#include <string>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

enum class Section {
	Main,
	UserConfig,
	GateMapConfig
};

bool main_show = true;
bool userconf_show = false;
bool gatemapconf_show = false;

void change_section(Section sect) {
	main_show = sect == Section::Main;
	userconf_show = sect == Section::UserConfig;
	gatemapconf_show = sect == Section::GateMapConfig;
}

int main() {
	using namespace ftxui;

	auto screen = ScreenInteractive::FitComponent();

	Section current_section = Section::Main;

	auto container = Container::Vertical({
		Container::Vertical({
			Button("Configure User", [&] { change_section(Section::UserConfig); }, ButtonOption::Animated(Color::Red)),
			Button("Configure Gate Map", [&] { change_section(Section::GateMapConfig); }, ButtonOption::Animated(Color::Green))
		}) | Maybe(&main_show),
		Container::Vertical({
			Button("Exit User Config", [&] { change_section(Section::Main); }, ButtonOption::Animated(Color::GrayDark))
		}) | Maybe(&userconf_show),
		Container::Vertical({
			Button("Exit Gate Map Config", [&] { change_section(Section::Main); }, ButtonOption::Animated(Color::GrayDark))
		}) | Maybe(&gatemapconf_show),
	});

	screen.Loop(container);

	return 0;
}