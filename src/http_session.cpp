#include "http_session.hpp"

http_session::http_session(
    tcp::socket&& socket,
    const std::shared_ptr<const std::string>& doc_root
) : stream(std::move(socket)),
    doc_root(doc_root)
{
    static_assert(queue_limit > 0, "queue limit must be non-zero and positive");
    response_queue.reserve(queue_limit);
}

void http_session::run() {
    net::dispatch(
        stream.get_executor(),
        beast::bind_front_handler(
            &http_session::do_read,
            shared_from_this()
        )
    );
}

void http_session::do_read() {
    // make a new parser for each request
    parser.emplace();

    // set a max body size of 10k to prevent abuse
    parser->body_limit(10000);

    stream.expires_after(std::chrono::seconds(30));

    http::async_read(stream, buffer, *parser,
        beast::bind_front_handler(
            &http_session::on_read,
            shared_from_this()
        )
    );
}

void http_session::on_read(
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    boost::ignore_unused(bytes_transferred);

    // if the client closed the connection
    if (ec == http::error::end_of_stream) {
        return do_close();
    }

    if (ec) {
        std::cerr << "Couldn't read an HTTP request from stream: " << ec.message() << std::endl;
        return;
    }

    // if the request is a WebSocket Upgrade
    if (websocket::is_upgrade(parser->get())) {
        // create a new websocket session, moving the socket and request into it
        std::make_shared<websocket_session>(
            stream.release_socket()
        )->do_accept(parser->release());
        return;
    }

    // send the response back
    queue_write(
        handle_request(*doc_root, parser->release())
    );

    // if the response queue is not at it's limit, try to add another response to the queue
    if (response_queue.size() < queue_limit) {
        do_read();
    }
}

void http_session::queue_write(http::message_generator msg) {
    // store the work
    response_queue.push_back(std::move(msg));

    // if there wasn't any work before, start the write loop
    if (response_queue.size() == 1) {
        do_write();
    }
}

bool http_session::do_write() {
    const bool was_full = response_queue.size() == queue_limit;

    if (!response_queue.empty()) {
        http::message_generator msg = std::move(response_queue.front());
        response_queue.erase(response_queue.begin());

        bool keep_alive = msg.keep_alive();

        beast::async_write(
            stream,
            std::move(msg),
            beast::bind_front_handler(
                &http_session::on_write,
                shared_from_this(),
                keep_alive
            )
        );
    }

    return was_full;
}

std::string path_cat(
    beast::string_view base,
    beast::string_view path
) {
    if (base.empty()) {
        return std::string(path);
    }
    std::string result(base);

#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    // if a path separator is present at the end of the base, remove it
    if (result.back() == path_separator) {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
    // replace any unix-like path separators with windows ones
    for (auto& c : result) {
        if (c == '/') {
            c = path_separator;
        }
    }
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator) {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
#endif
    return result;
}

beast::string_view mime_type(
    beast::string_view path
) {
    using beast::iequals;
    // get the extension part of the path (after the last dot)
    const auto ext = [&path]{
        const auto pos = path.rfind(".");
        // if there is no dot in the path, there is no extension
        if (pos == beast::string_view::npos) {
            return beast::string_view{};
        }
        return path.substr(pos);
    }();

    if (iequals(ext, ".htm"))       return "text/html";
    if (iequals(ext, ".html"))      return "text/html";
    if (iequals(ext, ".php"))       return "text/html";
    if (iequals(ext, ".css"))       return "text/css";
    if (iequals(ext, ".txt"))       return "text/plain";
    if (iequals(ext, ".js"))        return "application/javascript";
    if (iequals(ext, ".json"))      return "application/json";
    if (iequals(ext, ".xml"))       return "application/xml";
    if (iequals(ext, ".swf"))       return "application/x-shockwave-flash";
    if (iequals(ext, ".flv"))       return "video/x-flv";
    if (iequals(ext, ".png"))       return "image/png";
    if (iequals(ext, ".jpe"))       return "image/jpeg";
    if (iequals(ext, ".jpeg"))      return "image/jpeg";
    if (iequals(ext, ".jpg"))       return "image/jpeg";
    if (iequals(ext, ".gif"))       return "image/gif";
    if (iequals(ext, ".bmp"))       return "image/bmp";
    if (iequals(ext, ".ico"))       return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff"))      return "image/tiff";
    if (iequals(ext, ".tif"))       return "image/tiff";
    if (iequals(ext, ".svg"))       return "image/svg+xml";
    if (iequals(ext, ".svgz"))      return "image/svg+xml";
    return "application/text";
}

template <class Body, class Allocator>
http::message_generator handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req
) {
    const auto bad_request = 
        [&req] (beast::string_view why) {
            http::response<http::string_body> res{
                http::status::bad_request,
                req.version()
            };

            res.set(http::field::server, VERSION);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();

            return res;
        };
    
    const auto not_found = 
        [&req] (beast::string_view target) {
            http::response<http::string_body> res{
                http::status::not_found,
                req.version()
            };

            res.set(http::field::server, VERSION);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();

            return res;
        };

    const auto server_error = 
        [&req] (beast::string_view what) {
            http::response<http::string_body> res{
                http::status::internal_server_error,
                req.version()
            };

            res.set(http::field::server, VERSION);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "An error occured: '" + std::string(what) + "'";
            res.prepare_payload();

            return res;
        };

    // make sure we can handle the request
    if (
        req.method() != http::verb::get &&
        req.method() != http::verb::head
    ) {
        return bad_request("Unknown HTTP method");
    }

    // the request path must not contain ".." (parent dir) and be absolute
    if (
        req.target().empty() ||
        req.target()[0] != '/' || 
        req.target().find("..") != beast::string_view::npos
    ) {
        return bad_request("Illegal request target");
    }

    // build requested file path
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/') {
        path.append("index.html");
    }

    // open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // if the file doesn't exist, return the 404 error
    if (ec == beast::errc::no_such_file_or_directory) {
        return not_found(req.target());
    }

    // every other error is a server error
    if (ec) {
        return server_error(ec.message());
    }

    // save the size of the file for later
    const auto size = body.size();

    // if the request method is HEAD
    if (req.method() == http::verb::head) {
        http::response<http::empty_body> res{
            http::status::ok,
            req.version()
        };

        res.set(http::field::server, VERSION);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());

        return res;
    }

    // if the request method is GET
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())
    };

    res.set(http::field::server, VERSION);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

void http_session::on_write(
    bool keep_alive,
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        std::cerr << "Couldn't write to TCP stream: " << ec.message() << std::endl;
        return;
    }

    if (!keep_alive) {
        return do_close();
    }

    if (do_write()) {
        do_read();
    }
}

void http_session::do_close() {
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_send, ec);

    if (ec) {
        std::cerr << "Couldn't shutdown TCP connection: " << ec.message() << std::endl;
    }
}