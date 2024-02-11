#include "common_state.hpp"

common_state::common_state(
	net::io_context& io,
	std::shared_ptr<arduino_messenger> messenger
) : io(io.get_executor()), messenger(messenger) {}

void common_state::add_session(
	std::shared_ptr<websocket_session> session
) {
	sessions.push_back(session);
	messenger->send_message(json_message(json_message::QueryState, { 0, 1, 2, 3, 4, 5, 6 }));
}

void common_state::run() {
	update();
}

void common_state::update() {
	if (sessions.size() > 0) {
		// remove all dead sessions
		sessions.erase(
			std::remove_if(
				sessions.begin(),
				sessions.end(),
				[](std::weak_ptr<websocket_session>& s) { return s.expired(); }
			),
			sessions.end()
		);

		// update all sessions with new state
		if (!messenger->incoming_message_queue.empty()) {
			json_message message = messenger->incoming_message_queue.front();
			if (message.type == json_message::QueryStateResult) {
				for (auto& session : sessions) {
					if (std::shared_ptr<websocket_session> sp = session.lock()) {
						sp->queue_message(
							messenger->incoming_message_queue.front().dump_message()
						);
					}
				}
				messenger->incoming_message_queue.pop();
			}
		}
	}

	net::post(
		io,
		std::bind(
			&common_state::update,
			shared_from_this()
		)
	);
}
