#ifndef WEBSOCKET_SESSION_HPP
#define WEBSOCKET_SESSION_HPP

#include "common.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/asio/placeholders.hpp>
#include <memory>
#include <queue>
#include <thread>
#include "json_message.hpp"
#include "arduino_messenger.hpp"

class websocket_session : public std::enable_shared_from_this<websocket_session> {
    websocket::stream<beast::tcp_stream> ws;
    beast::flat_buffer buffer;
    std::string write_buffer;
    std::queue<std::string> write_queue;
    std::shared_ptr<arduino_messenger> arduino_connection;

public:
    explicit websocket_session(
        tcp::socket&& socket,
		std::shared_ptr<arduino_messenger> arduino_connection
    );

    template<class Body, class Allocator>
    void do_accept(
        http::request<Body, http::basic_fields<Allocator>> req
    ) {
        ws.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server
            )
        );

        // append a Server field to every response
        ws.set_option(
            websocket::stream_base::decorator(
                [](websocket::response_type& res) {
                    res.set(http::field::server, VERSION);
                }
            )
        );

        ws.async_accept(
            req,
            beast::bind_front_handler(
                &websocket_session::on_accept,
                shared_from_this()
            )
        );
    }

private:
    void on_accept(beast::error_code ec);
    void do_write();
    void do_read();
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    void queue_message(std::string_view message);

    void handle_message(std::string_view message);
};

#endif