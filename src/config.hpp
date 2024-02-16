#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <optional>

namespace fs = std::filesystem;

struct gc_config {
	struct parse_error : public std::runtime_error {
		parse_error(const char* why) : std::runtime_error(why) {}
	};

	std::string map_image_path;
	nlohmann::json gate_config;

	gc_config(
		std::string_view map_image_path,
		nlohmann::json gate_config
	) : map_image_path(map_image_path),
		gate_config(gate_config) {}

	static gc_config parse(std::string_view json) {
		nlohmann::json parsed_json = nlohmann::json::parse(json);

		if (!parsed_json.is_object()) {
			throw parse_error("root object is invalid");
		}

		if (parsed_json["mapImage"].is_null() || !parsed_json["mapImage"].is_string()) {
			throw parse_error("mapImage key is invalid");
		}

		if (parsed_json["gates"].is_null() || !parsed_json["gates"].is_array()) {
			throw parse_error("gates key is invalid");
		}

		return gc_config(parsed_json["mapImage"], parsed_json["gates"]);
	}

	static std::optional<gc_config> open_from_file(fs::path file_path) {
		std::ifstream file;
		file.open(file_path);
		if (!file.is_open()) {
			return std::nullopt;
		}

		std::stringstream ss;
		ss << file.rdbuf();

		std::string contents = ss.str();

		return parse(contents);
	}

	std::string get_gate_config_str() const {
		return gate_config.dump();
	}
};

#endif