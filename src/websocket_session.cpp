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

    ws.text(ws.got_text());

    ws.async_write(
        buffer.data(),
        beast::bind_front_handler(
            &websocket_session::on_write,
            shared_from_this()
        )
    );
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

    buffer.consume(buffer.size());

    // start another read
    do_read();
}