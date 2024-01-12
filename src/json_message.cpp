#include "json_message.hpp"

std::string json_message::type_to_str(const MessageType& type) {
	if (type == QueryState)				return "query_state";
	if (type == QueryStateResult)		return "query_state_result";
	if (type == ChangeState)			return "change_state";
	if (type == Availability)			return "availability";
	if (type == Text)					return "text";
}

json_message::MessageType json_message::str_to_type(const std::string_view str) {
	if (str == "query_state")			return QueryState;
	if (str == "query_state_result")	return QueryStateResult;
	if (str == "change_state")			return ChangeState;
	if (str == "availability")			return Availability;
	if (str == "text")					return Text;
}

std::string json_message::create_message(
	const MessageType type,
	const nlohmann::json payload
) {
	return nlohmann::json(
		{
			{ "type", type_to_str(type) },
			{ "payload", payload }
		}
	).dump();
}

std::string json_message::create_message(
	const std::pair<const MessageType, const nlohmann::json> msg
) {
	return create_message(msg.first, msg.second);
}

json_message json_message::parse_message(
	const std::string_view str
) {
	nlohmann::json parsed_json = nlohmann::json::parse(str);

	if (
		!parsed_json.is_object() ||
		!parsed_json["type"].is_string() ||
		(!parsed_json["payload"].is_primitive() && !parsed_json["payload"].is_structured())
	) {
		throw json_message_parse_error("malformed JSON data");
	}

	return json_message(
		json_message::str_to_type(parsed_json["type"].get<std::string>()),
		parsed_json["payload"]
	);
}

std::string json_message::dump_message() const {
	nlohmann::json json = {
		{ "type", type_to_str(type) },
		{ "payload", payload }
	};

	return json.dump();
}