#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

#include "common.hpp"
#include <memory>
#include <string>
#include <chrono>

beast::string_view mime_type(
    beast::string_view path
);

std::string path_cat(
    beast::string_view base,
    beast::string_view path
);

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
    http::request<http::string_body> req;

public:
    http_session(
        tcp::socket&& socket,
        const std::shared_ptr<const std::string>& doc_root
    );

    void run();

    void do_read();
    void on_read(
        beast::error_code ec, 
        std::size_t bytes_transferred
    );

    void send_response(http::message_generator&& msg);

    void do_close();

    void on_write(
        bool keep_alive,
        beast::error_code ec,
        std::size_t bytes_transferred
    );
};



#endif