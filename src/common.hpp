#ifndef COMMON_HPP
#define COMMON_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <iostream>
#include "version.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::beast::net;
using tcp = net::ip::tcp;
namespace websocket = beast::websocket;

#endif