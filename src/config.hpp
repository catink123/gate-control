#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <optional>
#include <initializer_list>
#include "auth.hpp"

namespace fs = std::filesystem;

struct map_entry {
	std::string id;
	std::optional<std::string> group;
	std::string map_image_path;
	nlohmann::json gate_config;

	map_entry(
		std::string id,
		std::optional<std::string> group,
		std::string map_image_path,
		nlohmann::json gate_config
	) : id(id), 
		group(group),
		map_image_path(map_image_path), 
		gate_config(gate_config) {}

	nlohmann::json to_json() const {
		nlohmann::json group_value = nullptr;
		if (group.has_value()) {
			group_value = group.value();
		}

		return {
			{ "id", id },
			{ "group", group_value },
			{ "mapImage", map_image_path },
			{ "gates", gate_config }
		};
	}

	nlohmann::json to_client_json() const {
		nlohmann::json group_value = nullptr;
		if (group.has_value()) {
			group_value = group.value();
		}

		return {
			{ "id", id },
			{ "group", group_value },
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
			(!entry["group"].is_string() && !entry["group"].is_null()) ||
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
			std::optional<std::string> group_value = std::nullopt;
			if (map["group"].is_string()) {
				group_value = map["group"];
			}

			maps.push_back(
				map_entry(
					map["id"],
					group_value,
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

	std::string get_maps_for_client(const auth_data& user_auth) const {
		std::vector<nlohmann::json> jsonified_maps;

		for (const map_entry& map : maps) {
			// if a map has an associated group 
			// and the user doesn't belong to that group, 
			// don't show it to the user with control permissions
			if (user_auth.permissions >= Control && map.group) {
				const auto& found_value =
					std::find(user_auth.map_groups.begin(), user_auth.map_groups.end(), map.group.value());

				if (found_value == user_auth.map_groups.end()) {
					continue;
				}
			}

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
};

#endif