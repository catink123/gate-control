#include "http_listener.hpp"

http_listener::http_listener(
    net::io_context& ioc,
    tcp::endpoint endpoint,
    const std::shared_ptr<const std::string>& doc_root
) : ioc(ioc),
    acceptor(net::make_strand(ioc)),
    doc_root(doc_root)
{
    beast::error_code ec;

    acceptor.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Couldn't open acceptor: " << ec.message() << std::endl;
        return;
    }

    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Couldn't set reuse_address: " << ec.message() << std::endl;
        return;
    }

    acceptor.bind(endpoint, ec);
    if (ec) {
        std::cerr << "Couldn't bind to endpoint: " << ec.message() << std::endl;
        return;
    }

    acceptor.listen(
        net::socket_base::max_listen_connections, ec
    );
    if (ec) {
        std::cerr << "Couldn't start listening: " << ec.message() << std::endl;
        return;
    }
}

void http_listener::run() {
    do_accept();
}

void http_listener::do_accept() {
    acceptor.async_accept(
        net::make_strand(ioc),
        beast::bind_front_handler(
            &http_listener::on_accept,
            shared_from_this()
        )
    );
}

void http_listener::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        std::cerr << "Couldn't accept incoming connection: " << ec.message() << std::endl;
        return;
    }

    std::make_shared<http_session>(
        std::move(socket),
        doc_root
    )->run();

    do_accept();
}