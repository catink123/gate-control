#ifndef COMMON_STATE_HPP
#define COMMON_STATE_HPP

#include <memory>
#include <vector>
#include "websocket_session.hpp"
#include <boost/asio/any_io_executor.hpp>
#include "arduino_messenger.hpp"

class common_state : public std::enable_shared_from_this<common_state> {
	net::any_io_executor io;
	std::vector<std::weak_ptr<websocket_session>> sessions;
	std::shared_ptr<arduino_messenger> messenger;

public:
	common_state(
		net::io_context& io,
		std::shared_ptr<arduino_messenger> messenger
	);

	void add_session(std::shared_ptr<websocket_session> session);

	void run();
	void update();
};

#endif