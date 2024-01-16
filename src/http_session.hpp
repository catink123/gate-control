#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

#include "common.hpp"
#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <chrono>
#include <array>
#include "websocket_session.hpp"
#include "arduino_messenger.hpp"
#include "common_state.hpp"
#include "auth.hpp"

namespace base64 = beast::detail::base64;

beast::string_view mime_type(
    beast::string_view path
);

std::string path_cat(
    beast::string_view base,
    beast::string_view path
);

const std::array<std::string, 3> indexable_endpoints = { "/", "/view", "/control" };

// handle given request by returning an appropriate response
template <class Body, class Allocator>
http::message_generator handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req
);

class http_session : public std::enable_shared_from_this<http_session> {
    beast::tcp_stream stream;
    beast::flat_buffer buffer;
    std::shared_ptr<const std::string> doc_root;

    std::shared_ptr<common_state> comstate;
    std::shared_ptr<arduino_messenger> arduino_connection;

    // a queue to prevent overload
    static constexpr std::size_t queue_limit = 16;
    std::vector<http::message_generator> response_queue;

    boost::optional<http::request_parser<http::string_body>> parser;

public:
    http_session(
        tcp::socket&& socket,
        const std::shared_ptr<const std::string>& doc_root,
		std::shared_ptr<common_state> comstate,
		std::shared_ptr<arduino_messenger> arduino_connection
    );

    void run();

    void do_read();
    void on_read(
        beast::error_code ec, 
        std::size_t bytes_transferred
    );

    void queue_write(http::message_generator msg);

    void do_close();

    bool do_write();
    void on_write(
        bool keep_alive,
        beast::error_code ec,
        std::size_t bytes_transferred
    );
};



#endif