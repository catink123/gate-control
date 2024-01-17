#include "auth.hpp"

std::optional<std::unordered_map<std::string, auth_data>> 
open_auth_table_from_file(fs::path file_path) {
    std::ifstream file;
    file.open(file_path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::unordered_map<std::string, auth_data> auth_table;
    std::string current_line;

    // parsing line by line with the following format:
    // login:permissions:password_hash
    while (std::getline(file, current_line)) {
        std::size_t first_delimeter = current_line.find(':');
        if (first_delimeter == std::string::npos) {
            continue;
        }

        std::size_t second_delimeter = current_line.find(':', first_delimeter + 1);
        if (second_delimeter == std::string::npos || current_line.size() == second_delimeter) {
            continue;
        }

        std::string login = current_line.substr(0, first_delimeter);
        std::string permissions_str = 
            current_line.substr(
                first_delimeter + 1, 
                second_delimeter - first_delimeter - 1
            );
        unsigned int permissions = 99;
        try {
            permissions = std::stoi(permissions_str);
        }
        catch (...) {
            continue;
        }
        if (permissions > 2) {
            continue;
        }

        std::string password_hash = current_line.substr(second_delimeter + 1);

        auth_table.insert({
            login,
            auth_data(
                static_cast<AuthorizationType>(permissions),
                password_hash
            ) 
		});
    }

    file.close();

    return auth_table;
}