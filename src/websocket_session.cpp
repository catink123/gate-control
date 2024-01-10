#include "websocket_session.hpp"

websocket_session::websocket_session(
    tcp::socket&& socket
) : ws(std::move(socket)) {}

void websocket_session::on_accept(beast::error_code ec) {
    if (ec) {
        std::cerr << "Couldn't accept a WebSocket request: " << ec.message() << std::endl;
        return;
    }

    // read the message
    do_read();
    
    // attempt to clear the write queue
    do_write();
}

void websocket_session::do_read() {
    // read into buffer
    ws.async_read(
        buffer,
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()
        )
    );
}

void websocket_session::do_write() {
    // if there is something to send, do it
    if (write_queue.size() > 0) {
        write_buffer = write_queue.front();
        write_queue.pop();

        ws.async_write(
            net::buffer(write_buffer),
            beast::bind_front_handler(
                &websocket_session::on_write,
                shared_from_this()
            )
        );
    }
    // if there isn't, call this function again asyncronously
    else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        net::post(
            ws.get_executor(),
            beast::bind_front_handler(
                &websocket_session::do_write,
                shared_from_this()
            )
        );
    }
}

void websocket_session::on_read(
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    boost::ignore_unused(bytes_transferred);

    if (ec == websocket::error::closed) {
        return;
    }

    if (ec) {
        std::cerr << "Couldn't read incoming WebSocket message: " << ec.message() << std::endl;
        return;
    }

    if (ws.got_text()) {
        auto buffer_data = buffer.data();
        std::string message(reinterpret_cast<const char*>(buffer_data.data()), buffer_data.size());

        handle_message(message);
    }

    buffer.consume(buffer.size());

    do_read();
}

void websocket_session::on_write(
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        std::cerr << "Couldn't write to a WebSocket stream: " << ec.message() << std::endl;
        return;
    }

    do_write();
}

void websocket_session::queue_message(std::string_view message) {
    write_queue.push(std::string(message));
}

void websocket_session::handle_message(std::string_view message) {
    try {
        auto parsed_msg = json_message::parse_message(message);

        if (parsed_msg.type == json_message::QueryState)
            queue_message(json_message::create_message(json_message::QueryStateResult));
        if (parsed_msg.type == json_message::ChangeState)
            queue_message(json_message::create_message(json_message::Text, "changing of state not implemented!"));
    }
    catch (...) {}
}