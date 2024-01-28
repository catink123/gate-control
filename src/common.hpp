#ifndef COMMON_HPP
#define COMMON_HPP

namespace boost {
	namespace asio {}
	namespace beast {
		namespace http {}
		namespace net = ::boost::asio;
		namespace websocket {}
	}
}

#include <iostream>
#include "version.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::beast::net;
namespace websocket = beast::websocket;

#endif