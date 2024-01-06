#ifndef HTTP_LISTENER_HPP
#define HTTP_LISTENER_HPP

#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include "common.hpp"
#include "http_session.hpp"

class http_listener : public std::enable_shared_from_this<http_listener> {
    net::io_context& ioc;
    tcp::acceptor acceptor;
    std::shared_ptr<const std::string> doc_root;
    
public:
    http_listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        const std::shared_ptr<const std::string>& doc_root
    );

    void run();

private:
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
};

#endif