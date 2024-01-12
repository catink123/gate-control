#ifndef ARDUINO_MESSENGER_HPP
#define ARDUINO_MESSENGER_HPP

#include "common.hpp"
#include <boost/asio/serial_port.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read_until.hpp>
#include "json_message.hpp"
#include <queue>
#include <thread>

class arduino_messenger : public std::enable_shared_from_this<arduino_messenger> {
	net::serial_port com;
	net::streambuf buffer;

	std::queue<json_message> outgoing_message_queue;
	std::mutex omq_mutex;

	std::string outgoing_message_buffer;

public:
	std::queue<json_message> incoming_message_queue;
	std::mutex imq_mutex;

	arduino_messenger(
		net::io_context& io,
		std::string_view device_name,
		unsigned int baud_rate = 9600
	);

	void send_message(json_message message);

	void run();

private:
	void do_read();
	void do_write();
	void on_read(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void on_write(const boost::system::error_code& ec, std::size_t bytes_transferred);
};

#endif