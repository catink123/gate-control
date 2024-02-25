#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <optional>
#include <initializer_list>

namespace fs = std::filesystem;

struct map_entry {
	std::string id;
	std::string map_image_path;
	nlohmann::json gate_config;

	map_entry(
		std::string id,
		std::string map_image_path,
		nlohmann::json gate_config
	) : id(id), 
		map_image_path(map_image_path), 
		gate_config(gate_config) {}

	nlohmann::json to_json() const {
		return {
			{ "id", id },
			{ "mapImage", map_image_path },
			{ "gates", gate_config }
		};
	}

	nlohmann::json to_client_json() const {
		return {
			{ "id", id },
			{ "gates", gate_config }
		};
	}
};

struct gc_config {
	struct parse_error : public std::runtime_error {
		parse_error(const char* why) : std::runtime_error(why) {}
	};

	std::vector<map_entry> maps;

	gc_config(std::initializer_list<map_entry> maps = {}) : maps(maps) {}
	gc_config(std::vector<map_entry> maps = {}) : maps(maps) {}

	static bool validate_gate_entry(nlohmann::json entry) {
		return
			entry.is_object() &&
			entry["x"].is_number_unsigned() &&
			entry["y"].is_number_unsigned() &&
			entry["id"].is_number_unsigned();
	}

	static bool validate_map_entry(nlohmann::json entry) {
		if (
			!entry.is_object() ||
			!entry["id"].is_string() ||
			!entry["mapImage"].is_string() ||
			!entry["gates"].is_array()
		) {
			return false;
		}

		for (auto gate : entry["gates"]) {
			if (!validate_gate_entry(gate)) {
				return false;
			}
		}

		return true;
	}

	static bool validate_config(nlohmann::json config_json) {
		if (!config_json.is_array()) {
			return false;
		}

		for (auto map : config_json) {
			if (!validate_map_entry(map)) {
				return false;
			}
		}

		return true;
	}

	static gc_config parse(std::string_view json) {
		nlohmann::json parsed_json = nlohmann::json::parse(json);

		if (!gc_config::validate_config(parsed_json)) {
			throw parse_error("config contents are malformed");
		}

		std::vector<map_entry> maps;

		for (auto map : parsed_json) {
			maps.push_back(
				map_entry(
					map["id"],
					map["mapImage"],
					map["gates"]
				)
			);
		}

		return gc_config(maps);
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

	std::string get_maps_for_client() const {
		std::vector<nlohmann::json> jsonified_maps;

		for (const map_entry& map : maps) {
			jsonified_maps.push_back(map.to_client_json());
		}

		return nlohmann::json(jsonified_maps).dump();
	}

	const map_entry& get_map_by_id(std::string_view id) {
		auto found_map = 
			std::find_if(
				maps.begin(), 
				maps.end(), 
				[&id](const map_entry& m) { return m.id == id; }
			);

		if (found_map == maps.end()) {
			throw std::runtime_error("couldn't find a map by given id");
		}

		return *found_map;
	}

	//std::string get_gate_config_str() const {
	//	return gate_config.dump();
	//}
};

#endif