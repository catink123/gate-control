#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

#include <memory>
#include <string>
#include <chrono>
#include <array>
#include <vector>
#include <semaphore>

#include "common.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/websocket/impl/rfc6455.hpp>
#include <boost/beast/core/string_type.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/buffers_generator.hpp>
#include <boost/beast/core/string_type.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/core/file_base.hpp>

#include <boost/asio/dispatch.hpp>

#include <boost/optional/optional_fwd.hpp>

#include "websocket_session.hpp"
#include "arduino_messenger.hpp"
#include "common_state.hpp"
#include "auth.hpp"
#include "config.hpp"

using tcp = net::ip::tcp;

beast::string_view mime_type(
    beast::string_view path
);

std::string path_cat(
    beast::string_view base,
    beast::string_view path
);

const std::array<std::string, 3> indexable_endpoints = { "/", "/view", "/control" };

template <class Body, class Allocator>
http::response<http::string_body> unauthorized_response(
    std::string& nonce,
    const std::string& opaque,
    http::request<Body, http::basic_fields<Allocator>>& req,
    beast::string_view target,
    bool stale = false
);

bool is_target_single_level(std::string_view target, std::string endpoint_name);

// handle given request by returning an appropriate response
template <class Body, class Allocator>
http::message_generator handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    std::shared_ptr<auth_table_t> auth_table,
    std::string& nonce,
    std::string& opaque,
    std::shared_ptr<gc_config> config
);

class http_session : public std::enable_shared_from_this<http_session> {
    beast::tcp_stream stream;
    beast::flat_buffer buffer;
    std::shared_ptr<const std::string> doc_root;

    std::shared_ptr<common_state> comstate;
    std::shared_ptr<arduino_messenger> arduino_connection;
    std::shared_ptr<auth_table_t> auth_table;

    // required for digest authentication
    std::shared_ptr<std::string> nonce;
    std::shared_ptr<std::string> opaque;

    // a queue to prevent overload
    static constexpr std::size_t queue_limit = 16;
    std::vector<http::message_generator> response_queue;

    boost::optional<http::request_parser<http::string_body>> parser;

    std::binary_semaphore write_semaphore{ 1 };

    std::shared_ptr<gc_config> config;

public:
    http_session(
        tcp::socket&& socket,
        std::shared_ptr<const std::string> doc_root,
		std::shared_ptr<common_state> comstate,
		std::shared_ptr<arduino_messenger> arduino_connection,
        std::shared_ptr<auth_table_t> auth_table,
        std::shared_ptr<std::string> opaque,
        std::shared_ptr<std::string> nonce,
        std::shared_ptr<gc_config> config
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