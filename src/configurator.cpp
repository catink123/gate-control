#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <filesystem>
#include "console_prettifier.hpp"
#include "auth_table.hpp"

namespace fs = std::filesystem;

typedef std::pair<std::string, std::function<void(void)>> menu_entry;

void fix_cin() {
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string to_lowercase(std::string input) {
	std::string transformed = input;

	std::transform(
		input.begin(),
		input.end(),
		transformed.begin(),
		[](const char& c) { return std::tolower(c); }
	);

	return transformed;
}

void menu(std::string title, std::vector<menu_entry> menu_items) {
	console::set_foreground_color(color::Cyan);
	std::cout << title << std::endl;
	console::reset_foreground_color();
	for (int i = 0; i < menu_items.size(); i++) {
		console::enable_bold();
		std::cout << i << ". "; 
		console::disable_bold();

		console::set_foreground_color(color::White);
		std::cout << menu_items[i].first << std::endl;
		console::reset_foreground_color();
	}

	std::cout << "Enter your choice: ";
	unsigned int choice;
	console::set_foreground_color(color::Green);
	std::cin >> choice;
	console::reset_foreground_color();
	while (std::cin.fail() || choice >= menu_items.size()) {
		console::set_foreground_color(color::Yellow);
		std::cout << "Invalid choice. Try again: ";
		console::set_foreground_color(color::Green);
		std::cin >> choice;
		console::reset_foreground_color();

		fix_cin();
	}

	menu_items[choice].second();
}

bool get_confirmation(std::string positive_answer = "y", std::string negative_answer = "n", bool nonintrusive = false) {
	std::string answer;
	std::getline(std::cin, answer);

	positive_answer = to_lowercase(positive_answer);
	bool is_positive = to_lowercase(answer).starts_with(positive_answer);

	if (nonintrusive) {
		return is_positive;
	}

	negative_answer = to_lowercase(negative_answer);
	bool is_negative = to_lowercase(answer).starts_with(negative_answer);

	while (!is_positive && !is_negative) {
		std::getline(std::cin, answer);
		is_positive = to_lowercase(answer).starts_with(positive_answer);
		is_negative = to_lowercase(answer).starts_with(negative_answer);
	}

	return is_positive;
}

struct state {
	std::optional<fs::path> auth_file_path;
	std::optional<auth_table_t> auth_table;
	std::optional<fs::path> config_file_path;
};

template <typename T>
std::string stringify_vector(std::vector<T> v) {
	if (v.size() == 0) {
		return "empty";
	}

	std::string result;

	for (int i = 0; i < v.size(); i++) {
		result += v[i];

		if (i != v.size() - 1) {
			result += ", ";
		}
		else {
			result += ".";
		}
	}

	return result;
}

void print_auth_table(const auth_table_t& table) {
	for (auto it = table.begin(); it != table.end(); it++) {
		const auto& pair = *it;

		const auth_data& data = pair.second;

		console::enable_bold();
		std::cout << "Login: ";
		console::disable_bold();

		console::set_foreground_color(color::White);
		std::cout << pair.first << std::endl;
		console::reset_foreground_color();

		console::enable_bold();
		std::cout << "Password: ";
		console::disable_bold();

		console::set_foreground_color(color::White);
		std::cout << data.password << std::endl;
		console::reset_foreground_color();

		console::enable_bold();
		std::cout << "Permissions: ";
		console::disable_bold();

		console::set_foreground_color(color::White);
		std::cout << auth_type_to_str(data.permissions) << " (" << data.permissions << ")" << std::endl;
		console::reset_foreground_color();

		console::enable_bold();
		std::cout << "Map groups: ";
		console::disable_bold();

		console::set_foreground_color(color::White);
		std::cout << stringify_vector(data.map_groups) << std::endl;
		console::reset_foreground_color();

		if (it != --table.end()) {
			std::cout << std::endl;
		}
	}
}

void auth_menu(state& s) {
	bool menu_loop = true;

	const auto set_file_path =
		[&s] {
			fs::path path;
			std::cout << "Enter file path: ";

			std::cin >> path;

			while (std::cin.fail()) {
				console::set_foreground_color(color::Yellow);
				std::cout << "Invalid file path. Try again: ";
				console::reset_foreground_color();
				std::cin >> path;
			}

			fix_cin();

			if (!fs::exists(path)) {
				std::cout << "The auth-file doesn't exist. Do you want to create it? (y/n) ";
				if (get_confirmation()) {
					s.auth_table.emplace();
				}
				else {
					std::cout << "The auth-file on the entered path doesn't exist. Returning to menu..." << std::endl;
					return;
				}
			}
			else {
				s.auth_table = open_auth_table_from_file(path);
			}

			s.auth_file_path = path;
		};

	const auto print_auth_entries =
		[&s] {
			if (!s.auth_table) {
				console::set_foreground_color(color::Yellow);
				std::cout << "No Auth-file is open. Set the file path." << std::endl;
				console::reset_foreground_color();

				return;
			}

			print_auth_table(s.auth_table.value());
		};

	const auto add_new_user =
		[&s] {
			if (!s.auth_table) {
				console::set_foreground_color(color::Yellow);
				std::cout << "No Auth-file is open. Set the file path." << std::endl;
				console::reset_foreground_color();

				return;
			}

			fix_cin();

			std::cout << "Enter login: ";
			std::string login;
			std::getline(std::cin, login);
			while (login.size() == 0 || s.auth_table->find(login) != s.auth_table->end()) {
				console::set_foreground_color(color::Yellow);
				std::cout << "Invalid login! Try again: ";
				console::reset_foreground_color();

				std::getline(std::cin, login);
			}

			std::cout << "Enter password: ";
			std::string password;
			std::getline(std::cin, password);
			while (password.size() == 0) {
				console::set_foreground_color(color::Yellow);
				std::cout << "Password cannot be empty! Try again: ";
				console::reset_foreground_color();

				std::getline(std::cin, password);
			}

			std::cout << "Enter permission category (0 - Blocked, 1 - View, 2 - Control): ";
			std::string permissions_str;
			std::getline(std::cin, permissions_str);

			unsigned int permissions = 99;
			bool conversion_successful = false;
			while (!conversion_successful) {
				try {
					permissions = std::stoi(permissions_str);
					conversion_successful = true;
				}
				catch (...) {
					conversion_successful = false;
				}
				if (permissions > 2) {
					conversion_successful = false;
				}

				if (!conversion_successful) {
					console::set_foreground_color(color::Yellow);
					std::cout << "Invalid permission category! Try again: ";
					console::reset_foreground_color();
					std::getline(std::cin, permissions_str);
				}
			}

			std::cout << "Enter map groups (separated by semicolon, leave empty for no categories): ";
			std::string map_groups_str;
			std::getline(std::cin, map_groups_str);

			std::vector<std::string> map_groups = split_str(map_groups_str);

			s.auth_table->insert(
				{ 
					login, 
					auth_data(static_cast<AuthorizationType>(permissions), map_groups, password) 
				}
			);
		};

	const auto delete_user =
		[&s] {
			if (!s.auth_table) {
				console::set_foreground_color(color::Yellow);
				std::cout << "No Auth-file is open. Set the file path." << std::endl;
				console::reset_foreground_color();

				return;
			}

			fix_cin();

			std::cout << "Enter login: ";
			std::string login;
			std::getline(std::cin, login);
			while (login.size() == 0) {
				console::set_foreground_color(color::Yellow);
				std::cout << "Login cannot be empty! Try again: ";
				console::reset_foreground_color();

				std::getline(std::cin, login);
			}

			auto found_user = s.auth_table->find(login);

			if (found_user == s.auth_table->end()) {
				std::cout << "User with specified login wasn't found. Returning to menu..." << std::endl;
				return;
			}

			s.auth_table->erase(found_user);
			std::cout << "User was successfully deleted." << std::endl;
		};

	const auto write_to_disk =
		[&s] {
			if (!s.auth_table) {
				console::set_foreground_color(color::Yellow);
				std::cout << "No Auth-file is open. Set the file path." << std::endl;
				console::reset_foreground_color();

				return;
			}

			try {
				save_auth_table_to_file(s.auth_file_path.value(), s.auth_table.value());
			}
			catch (...) {
				console::set_foreground_color(color::Red);
				std::cout << "Couldn't save the Auth-file!" << std::endl;
				console::reset_foreground_color();
				return;
			}

			std::cout << "Auth-file was successfully saved." << std::endl;
		};

	while (menu_loop) {
		std::cout << std::endl;

		std::string title = "Editing Auth-file";

		if (s.auth_file_path) {
			title += " (";
			title += s.auth_file_path.value().string();
			title += ")";
		}

		menu(
			title,
			{
				{ "Exit", [&menu_loop] { menu_loop = false; } },
				{ "Set file path", set_file_path },
				{ "Print Auth-file entires", print_auth_entries },
				{ "Add a new user", add_new_user },
				{ "Delete user", delete_user },
				{ "Write Auth-file to disk", write_to_disk }
			}
		);
	}
}

//void config_menu(state& s) {
//	bool menu_loop = true;
//
//	while (menu_loop) {
//		std::cout << std::endl;
//
//		std::cout << "Config file: " << s.config_file_path.value_or("None") << std::endl;
//
//		menu(
//			"Editing config file",
//			{
//				{ "Exit", [&menu_loop] { menu_loop = false; } },
//				{ "Set file path", [&s] {
//					fs::path path;
//					std::cout << "Enter file path: ";
//
//					std::cin >> path;
//
//					while (std::cin.fail() || !fs::exists(path)) {
//						console::set_foreground_color(color::Yellow);
//						std::cout << "Invalid file path. Try again: ";
//						console::reset_foreground_color();
//						std::cin >> path;
//					}
//
//					s.config_file_path = path;
//				} },
//			}
//		);
//	}
//}

void main_menu(state& s) {
	bool menu_loop = true;

	while (menu_loop) {
		std::cout << std::endl;

		menu(
			"Main menu", 
			{
				{ "Exit", [&menu_loop] { menu_loop = false; } },
				{ "Edit Auth-file", [&s] { auth_menu(s); } },
				//{ "Edit config file", [&s] { config_menu(s); } },
			}
		);
	}
}

int main() {
	state s;
	main_menu(s);

	return 0;
}