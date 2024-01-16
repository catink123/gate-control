#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

#include "common.hpp"
#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <chrono>
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

// catink123:testpassword123
// guest:guest
const std::unordered_map<std::string, auth_data> temp_auth_table = {
    { "catink123", auth_data(Control, "$2a$10$o12u27uUOjD6rJ0dlEE/EuL8EqGa7y8iwZqAp3wF0WBS4.Vu/9jhK") },
    { "guest", auth_data(View, "$2a$10$vYQHg8mBFTle1OzRp31MsOMvrmfQ52xfHUGFoi3aTe6Vp8GhDRzBy") }
};

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