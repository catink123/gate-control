#include "arduino_messenger.hpp"

arduino_messenger::arduino_messenger(
	net::io_context& io,
	std::string_view device_name,
	unsigned int baud_rate
) : com(io)
{
	boost::system::error_code error;
	com.open(std::string(device_name), error);

	if (error) {
		if (error == boost::system::errc::device_or_resource_busy)
			throw open_error("serial port already in use");
		if (error == boost::system::errc::no_such_file_or_directory)
			throw open_error("serial port doesn't exist");
		throw open_error(error.message().c_str());
	}

	com.set_option(
		net::serial_port_base::baud_rate(baud_rate)
	);
	// enable DTR (Data Terminal Ready) for reading from the COM-port to work
	com.set_option(
		net::serial_port_base::flow_control(
			net::serial_port_base::flow_control::hardware
		)
	);
}

void arduino_messenger::run() {
	// read from COM-port indefinitely
	do_read();

	// try to empty outgoing message queue
	do_write();
}

void arduino_messenger::do_read() {
	net::streambuf::mutable_buffers_type temp_mut_buf = buffer.prepare(MAX_MESSAGE_LENGTH);

	com.async_read_some(net::buffer(temp_mut_buf),
		beast::bind_front_handler(
			&arduino_messenger::on_read,
			shared_from_this()
		)
	);
}

void arduino_messenger::on_read(
	const boost::system::error_code& ec,
	std::size_t bytes_transferred
) {
	// this doesn't work
	if (ec && !com.is_open()) {
		std::cout << "COM-port closed." << std::endl;
	}

	if (ec) {
		return;
	}

	if (bytes_transferred == 0) {
		do_read();
		return;
	}

	buffer.commit(bytes_transferred);

	std::string message(
		reinterpret_cast<const char*>(buffer.data().data()), 
		buffer.data().size()
	);

	buffer.consume(bytes_transferred);

	std::lock_guard lock(imq_mutex);
	incoming_message_queue.push(json_message::parse_message(message));

	do_read();
}

void arduino_messenger::do_write() {
	if (!outgoing_message_queue.empty()) {
		outgoing_message_buffer = outgoing_message_queue.front().dump_message();
		outgoing_message_queue.pop();

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
		// this doesn't work, else-clause called
		if (!com.is_open()) {
			std::cerr << "COM-port closed." << std::endl;
		}
		else {
			std::cerr << "Couldn't write to COM-port: " << ec.message() << std::endl;
		}
		return;
	}

	do_write();
}

void arduino_messenger::send_message(json_message message) {
	std::lock_guard lock(omq_mutex);
	outgoing_message_queue.push(message);
}
