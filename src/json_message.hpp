#ifndef JSON_MESSAGE_HPP
#define JSON_MESSAGE_HPP

#include <nlohmann/json.hpp>
#include <string>

class json_message {
public:
	enum MessageType {
		QueryState,
		QueryStateResult,
		ChangeState,
		Availability,
		Text
	};
	
	class json_message_parse_error : std::runtime_error {
	public:
		json_message_parse_error(const char* what) : std::runtime_error(what) {}
	};

	MessageType type;
	nlohmann::json payload;

	json_message(
		const MessageType type,
		nlohmann::json payload = nullptr
	) : type(type), payload(payload) {}

	static std::string type_to_str(const MessageType& type);	
	static MessageType str_to_type(const std::string_view str);

	static std::string create_message(
		const MessageType type,
		const nlohmann::json payload = nullptr
	);

	static std::string create_message(
		const std::pair<const MessageType, const nlohmann::json> msg
	);

	static json_message parse_message(const std::string_view str);
	std::string dump_message() const;
};

#endif