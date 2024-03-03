#include "http_session.hpp"

http_session::http_session(
    tcp::socket&& socket,
    std::shared_ptr<const std::string> doc_root,
    std::shared_ptr<common_state> comstate,
    std::shared_ptr<arduino_messenger> arduino_connection,
    std::shared_ptr<auth_table_t> auth_table,
    std::shared_ptr<std::string> opaque,
    std::shared_ptr<std::string> nonce,
    std::shared_ptr<gc_config> config
) : stream(std::move(socket)),
    doc_root(doc_root),
    comstate(comstate),
    arduino_connection(arduino_connection),
    auth_table(auth_table),
    opaque(opaque),
    nonce(nonce),
    config(config)
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
        // make sure the authentication is valid (it is most definitely not)

        auto req = parser->release();
        const std::string auth_field = req.at(http::field::authorization);
        const auto digest_opt = parse_digest_auth_field(auth_field);
        if (!digest_opt) {
            queue_write(
                unauthorized_response(*nonce, *opaque, req, req.target(), false)
            );

            return;
        }

        const std::string& request_nonce = digest_opt.value().nonce;
        if (request_nonce != *nonce) {
            queue_write(
                unauthorized_response(*nonce, *opaque, req, req.target(), true)
            );

            return;
        }

        // create a new websocket session, moving the socket and request into it
        auto session = 
            std::make_shared<websocket_session>(
				stream.release_socket(),
                arduino_connection
			);

        session->do_accept(req, auth_table, *nonce, *opaque);
        comstate->add_session(session);
        
        return;
    }

    // send the response back
    queue_write(
        handle_request(*doc_root, parser->release(), auth_table, *nonce, *opaque, config)
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

        // prevent multiple simultaneous async writes
        write_semaphore.acquire();

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
http::response<http::string_body> unauthorized_response(
    std::string& nonce,
    const std::string& opaque,
    http::request<Body, http::basic_fields<Allocator>>& req,
    beast::string_view target,
    bool stale
) {
	// generate a new nonce for a 401 status
	nonce = generate_base64_str(NONCE_SIZE);

	http::response<http::string_body> res{
		http::status::unauthorized,
		req.version()
	};

	res.set(http::field::server, VERSION);
	res.set(
		http::field::www_authenticate,
		generate_digest_response(nonce, opaque, stale)
	);
	res.keep_alive(req.keep_alive());
	res.body() = "Unauthorized client on resource '" + std::string(target) + "'.";
	res.prepare_payload();

	return res;
}

bool is_target_single_level(std::string_view target, std::string endpoint_name) {
    return
        target.starts_with("/" + endpoint_name) &&
        (target.ends_with(endpoint_name + "/") || target.ends_with(endpoint_name));
}

bool target_starts_with_segment(std::string_view target, std::string endpoint_name) {
    std::string starting_part = "/" + endpoint_name;
    if (target.starts_with(starting_part)) {
        if (starting_part.size() == target.size()) {
            return true;
        }

        return target[starting_part.size()] == '/';
    }

    return false;
}

template <class Body, class Allocator>
http::message_generator handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    std::shared_ptr<auth_table_t> auth_table,
    std::string& nonce,
    std::string& opaque,
    std::shared_ptr<gc_config> config
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

    const auto forbidden =
        [&req](beast::string_view target) {
			http::response<http::string_body> res{
				http::status::forbidden,
				req.version()
			};

			res.set(http::field::server, VERSION);
			res.keep_alive(req.keep_alive());
			res.body() = "Access to resource '" + std::string(target) + "' is forbidden";
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

    // if the request target is not the root page...
    if (const auto endpoint_perms = get_endpoint_permissions(req.target())) {
        if (endpoint_perms.value() == Blocked) {
            return forbidden(req.target());
        }

		// make sure the client has sufficient permissions
		if (
			req.find(http::field::authorization) == req.end()
		) {
            return unauthorized_response(nonce, opaque, req, req.target());
		}

		const auto auth = get_auth(req, *auth_table, nonce, opaque);

		if (!auth) {
            return unauthorized_response(nonce, opaque, req, req.target());
		}

        if (auth->permissions < endpoint_perms.value()) {
            return forbidden(req.target());
        }
    }

    std::string path;

    if (is_target_single_level(req.target(), "config")) {
		std::optional<auth_data> user_auth_data = get_auth(req, *auth_table, nonce, opaque);

        std::string map_config = config->get_maps_for_client(user_auth_data.value());

        if (req.method() == http::verb::head) {
			http::response<http::empty_body> res{
				http::status::ok,
				req.version()
			};

			res.set(http::field::server, VERSION);
			res.set(http::field::content_type, "application/json");
			res.content_length(map_config.size());
			res.keep_alive(req.keep_alive());

			return res;
        }
        else if (req.method() == http::verb::get) {
            http::response<http::string_body> res{
                http::status::ok,
                req.version()
            };

			res.set(http::field::server, VERSION);
			res.set(http::field::content_type, "application/json");
			res.content_length(map_config.size());
			res.keep_alive(req.keep_alive());
            res.body() = map_config;

            return res;
        }
        else {
            return bad_request("Invalid method on /config");
        }
    }
    if (target_starts_with_segment(req.target(), "maps")) {
        // determine which map to send to the client
        if (req.target() == "/maps" || req.target() == "/maps/") {
            return not_found(req.target());
        }

        std::string id = req.target().substr(6);

        try {
			path = config->get_map_by_id(id).map_image_path;
        }
        catch (...) {
            return not_found(req.target());
        }
    }
    else {
		// build requested file path
		path = path_cat(doc_root, req.target());

		if (
			req.target().back() == '/'
		) {
			path.append("index.html");
		}
		else if (
			std::find(
				indexable_endpoints.begin(),
				indexable_endpoints.end(),
				req.target()
			) != indexable_endpoints.end()
		) {
			path.append("/index.html");
		}
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
    write_semaphore.release();
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