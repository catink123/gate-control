#include <nlohmann/json.hpp>
#include <boost/asio/signal_set.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include "common.hpp"
#include "http_listener.hpp"
#include "common_state.hpp"

const auto ADDRESS = net::ip::make_address_v4("0.0.0.0");
const auto PORT = static_cast<unsigned short>(80);
const auto DOC_ROOT = std::make_shared<std::string>("./client");
const auto THREAD_COUNT = 8;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <com-port>", argv[0]);
        return 1;
    }

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
			tcp::endpoint{ADDRESS, PORT},
			DOC_ROOT,
			comstate,
			arduino_connection
		)->run();

		std::cout << "Server started at " << ADDRESS << ":" << PORT << "." << std::endl;

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
					ioc.run();
				}
			);
		}
		ioc.run();

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