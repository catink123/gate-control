#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <fstream>
#include <filesystem>

enum AuthorizationType {
    Blocked = 0,
    View = 1,
    Control = 2,
};

std::string auth_type_to_str(AuthorizationType type) {
    if (type == Blocked)    return "Blocked";
    if (type == View)       return "View";
    if (type == Control)    return "Control";
    throw std::runtime_error("invalid AuthorizationType");
}

struct auth_data {
    AuthorizationType permissions;
    const std::vector<std::string> map_groups;
    std::string password;

    auth_data(
        const AuthorizationType permissions,
        const std::vector<std::string> map_groups,
        std::string_view password
    ) : permissions(permissions), map_groups(map_groups), password(password) {}
};

typedef std::unordered_map<std::string, auth_data> auth_table_t;

std::vector<std::string> split_str(std::string input, char delimeter = ';') {
	std::vector<std::string> result;
    if (input.size() == 0) {
        return result;
    }

	std::size_t previous_delim_idx = 0;
	std::size_t delim_idx;
	do {
		delim_idx = input.find(delimeter, previous_delim_idx);
		std::string value = 
			input.substr(previous_delim_idx, delim_idx - previous_delim_idx);

		result.push_back(value);

		previous_delim_idx = delim_idx + 1;
	} while (delim_idx != std::string::npos);

    return result;
}

std::string join_vector(const std::vector<std::string>& vec, char delimeter = ';') {
    std::string result;

    for (auto it = vec.begin(); it != vec.end(); it++) {
        result += *it;
        if (it != --vec.end()) {
            result += delimeter;
        }
    }

    return result;
}

std::optional<std::unordered_map<std::string, auth_data>>
open_auth_table_from_file(std::filesystem::path file_path) {
    std::ifstream file;
    file.open(file_path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::unordered_map<std::string, auth_data> auth_table;
    std::string current_line;

    // parsing line by line with the following format:
    // login:permissions:map_groups:password
    // map_groups is an array of ids delimeted by semicolons
    while (std::getline(file, current_line)) {
        // delimits login and permissions
        std::size_t first_delimeter = current_line.find(':');
        if (first_delimeter == std::string::npos) {
            continue;
        }

        // delimits permissions and map_groups
        std::size_t second_delimeter = current_line.find(':', first_delimeter + 1);
        if (second_delimeter == std::string::npos || current_line.size() == second_delimeter) {
            continue;
        }

        // delimits map_groups and password
        std::size_t third_delimeter = current_line.find(':', second_delimeter + 1);
        if (third_delimeter == std::string::npos || current_line.size() == third_delimeter) {
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

        std::string map_groups_str =
            current_line.substr(
                second_delimeter + 1,
                third_delimeter - second_delimeter - 1
            );

        std::vector<std::string> map_groups = split_str(map_groups_str);

        std::string password = current_line.substr(third_delimeter + 1);

        auth_table.insert({
            login,
            auth_data(
                static_cast<AuthorizationType>(permissions),
                map_groups,
                password
            ) 
		});
    }

    file.close();

    return auth_table;
}

void save_auth_table_to_file(std::filesystem::path path, const auth_table_t& table) {
    std::ofstream file;
    file.open(path, std::ofstream::trunc | std::ofstream::out);
    if (!file.is_open()) {
        throw std::runtime_error("couldn't open file for writing");
    }

    for (auto it = table.begin(); it != table.end(); it++) {
        const auto& pair = *it;
        const auth_data& data = pair.second;
        file <<
            pair.first << ":" <<
            data.permissions << ":" <<
            join_vector(data.map_groups) << ":" <<
            data.password;

        if (it != --table.end()) {
            file << std::endl;
        }
    }
}
