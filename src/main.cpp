#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include "common.hpp"
#include "http_listener.hpp"

const auto ADDRESS = net::ip::make_address_v4("0.0.0.0");
const auto PORT = static_cast<unsigned short>(80);
const auto DOC_ROOT = std::make_shared<std::string>("./client");
const auto THREAD_COUNT = 8;

int main() {
    net::io_context ioc{THREAD_COUNT};

    std::make_shared<http_listener>(
        ioc,
        tcp::endpoint{ADDRESS, PORT},
        DOC_ROOT
    )->run();

    std::cout << "Server started at " << ADDRESS << ":" << PORT << "." << std::endl;

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

    return 0;
}