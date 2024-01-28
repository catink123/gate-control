#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <limits>

#include <nlohmann/json.hpp>

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "http_listener.hpp"
#include "common_state.hpp"

using tcp = net::ip::tcp;

const auto DEFAULT_ADDRESS = net::ip::make_address_v4("0.0.0.0");
const auto DEFAULT_PORT = static_cast<unsigned short>(80);
const auto DOC_ROOT = std::make_shared<std::string>("./client");
const auto THREAD_COUNT = 8;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <com-port> <auth-file> [<ipv4-address>] [<port>]", argv[0]);
        return 1;
    }

	std::optional<net::ip::address_v4> address;
	std::optional<unsigned short> port;

	if (argc == 4) {
		try {
			address = net::ip::make_address_v4(argv[3]);
		}
		catch (...) {
			std::cerr << "Invalid IPv4 Address passed in an argument." << std::endl;
			return 1;
		}
	}

	if (!address) {
		address = DEFAULT_ADDRESS;
	}

	if (argc == 5) {
		try {
			int port_int = std::stoi(argv[4]);
			if (port_int > std::numeric_limits<unsigned short>::max() || port_int < 0) {
				std::cerr << "Port passed as an argument is invalid." << std::endl;
				return 1;
			}
			port = static_cast<unsigned short>(port_int);
		}
		catch (...) {
			std::cerr << "Port passed as an argument is invalid." << std::endl;
			return 1;
		}
	}

	if (!port) {
		port = DEFAULT_PORT;
	}

	fs::path auth_file_path;

	try {
		auth_file_path = argv[2];
	}
	catch (...) {
		std::cerr << "Invalid auth-file path." << std::endl;
		return 1;
	}

	auto auth_table = open_auth_table_from_file(auth_file_path);
	if (!auth_table) {
		std::cerr << "Couldn't open the supplied auth-file." << std::endl;
		return 1;
	}

	std::shared_ptr<auth_table_t> auth_table_ptr = 
		std::make_shared<auth_table_t>(std::move(auth_table.value()));

    net::io_context ioc{THREAD_COUNT};

    try {
		auto arduino_connection =
			std::make_shared<arduino_messenger>(
				ioc,
				argv[1],
				115200
			);

		arduino_connection->run();

		auto comstate = 
			std::make_shared<common_state>(
				ioc,
				arduino_connection
			);

		comstate->run();

		std::make_shared<http_listener>(
			ioc,
			tcp::endpoint{address.value(), port.value()},
			DOC_ROOT,
			comstate,
			arduino_connection,
			auth_table_ptr
		)->run();

		std::cout << "Server started at " << DEFAULT_ADDRESS << ":" << DEFAULT_PORT << "." << std::endl;

		// graceful shutdown
		net::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait(
			[&](const beast::error_code&, int) {
				// abort all operations
				ioc.stop();
				std::cout << "Server is stopping..." << std::endl;
			}
		);

		std::vector<std::thread> v;
		v.reserve(THREAD_COUNT - 1);
		for (auto i = 0; i < THREAD_COUNT - 1; ++i) {
			v.emplace_back(
				[&ioc] {
					try {
						ioc.run();
					}
					catch (const std::exception& ex) {
						std::cerr << "Stopping thread because of an unhandled exception: " << ex.what() << std::endl;
					}
				}
			);
		}
		try {
			ioc.run();
		}
		catch (const std::exception& ex) {
			std::cerr << "Stopping main thread because of an unhandled exception: " << ex.what() << std::endl;
		}

		// if the program is here, the graceful shutdown is in progress, wait for all threads to end
		for (std::thread& th : v) {
			th.join();
		}
    }
    catch (const arduino_messenger::open_error& error) {
        std::cerr << "Couldn't connect to the Arduino: " << error.what() << std::endl;
        return 1;
    }
	return 0;
}