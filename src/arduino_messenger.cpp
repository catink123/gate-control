#include "arduino_messenger.hpp"

arduino_messenger::arduino_messenger(
	net::io_context& io,
	std::string_view device_name,
	unsigned int baud_rate
) : com(io)
{
	com.open(std::string(device_name));
	com.set_option(net::serial_port_base::baud_rate(baud_rate));
}

void arduino_messenger::run() {
	// read from COM-port indefinitely
	do_read();

	// try to empty outgoing message queue
	do_write();
}

void arduino_messenger::do_read() {
	net::async_read_until(com, buffer, '%',
		std::bind(
			&arduino_messenger::on_read,
			shared_from_this(),
			net::placeholders::error,
			net::placeholders::bytes_transferred
		)
	);
}

void arduino_messenger::on_read(
	const boost::system::error_code& ec,
	std::size_t bytes_transferred
) {
	if (ec || bytes_transferred == 0) {
		return;
	}

	auto buffer_data = buffer.data();

	std::string message(reinterpret_cast<const char*>(buffer_data.data()), buffer_data.size());

	incoming_message_queue.push(json_message::parse_message(message));
}

void arduino_messenger::do_write() {
	if (!outgoing_message_queue.empty()) {
		outgoing_message_buffer = outgoing_message_queue.front().dump_message() + '%';
		outgoing_message_queue.pop();

		std::cout << "Sending message: " << outgoing_message_buffer << std::endl;

		net::async_write(
			com,
			net::buffer(outgoing_message_buffer),
			beast::bind_front_handler(
				&arduino_messenger::on_write,
				shared_from_this()
			)
		);
	}
	else {
		net::post(
			com.get_executor(),
			std::bind(
				&arduino_messenger::do_write,
				shared_from_this()
			)
		);
	}
}

void arduino_messenger::on_write(
	const boost::system::error_code& ec,
	std::size_t bytes_transferred
) {
	boost::ignore_unused(bytes_transferred);

	outgoing_message_buffer.clear();

	if (ec) {
		std::cerr << "Couldn't write to COM-port: " << ec.message() << std::endl;
		return;
	}

	do_write();
}

void arduino_messenger::send_message(json_message message) {
	std::lock_guard lock(omq_mutex);
	outgoing_message_queue.push(message);
}
