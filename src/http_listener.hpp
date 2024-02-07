#ifndef HTTP_LISTENER_HPP
#define HTTP_LISTENER_HPP

#include "common.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/asio/strand.hpp>

#include <boost/beast/core/error.hpp>
#include <boost/beast/core/bind_handler.hpp>

#include <nlohmann/json.hpp>

#include "http_session.hpp"
#include "common_state.hpp"

using tcp = net::ip::tcp;

class http_listener : public std::enable_shared_from_this<http_listener> {
    static constexpr std::size_t MAX_USED_NONCES = 20;

    net::io_context& ioc;
    tcp::acceptor acceptor;
    std::shared_ptr<const std::string> doc_root;
    std::shared_ptr<common_state> comstate;
    std::shared_ptr<arduino_messenger> arduino_connection;
    std::shared_ptr<auth_table_t> auth_table;

    std::shared_ptr<std::string> opaque;
    std::unordered_map<net::ip::address, std::shared_ptr<std::string>> associated_nonces;
    
public:
    http_listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        std::shared_ptr<const std::string> doc_root,
        std::shared_ptr<common_state> comstate,
        std::shared_ptr<arduino_messenger> arduino_connection,
        std::shared_ptr<auth_table_t> auth_table
    );

    void run();

private:
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
};

#endif