#ifndef AUTH_HPP
#define AUTH_HPP

#include "common.hpp"
#include <unordered_map>
#include <optional>
#include <bcrypt.h>
#include <boost/beast/core/detail/base64.hpp>
#include <array>
#include <unordered_map>
#include <filesystem>
#include <fstream>

namespace base64 = beast::detail::base64;
namespace fs = std::filesystem;

enum AuthorizationType {
    Blocked = 0,
    View = 1,
    Control = 2,
};

const std::unordered_map<std::string, std::optional<AuthorizationType>> endpoint_map = {
    { "/", std::nullopt },
    { "/control", Control },
    { "/view", View }
};

template <class Body, class Allocator>
std::optional<AuthorizationType> get_endpoint_permissions(
	const http::request<Body, http::basic_fields<Allocator>>& req
) {
    auto& target = req.target();
    std::size_t last_delimeter = target.rfind('/');
    std::string endpoint;
    if (last_delimeter == 0) {
        if (target.size() > 1) {
            endpoint = target;
        }
        else {
			endpoint = "/";
        }
    }
    else {
        endpoint = target.substr(0, last_delimeter);
    }

    if (endpoint_map.find(endpoint) == endpoint_map.end()) {
        return Blocked;
    }

    return endpoint_map.at(endpoint);
}

struct auth_data {
    AuthorizationType permissions;
    std::string password_hash;

    auth_data(
        const AuthorizationType permissions,
        std::string_view password_hash
    ) : permissions(permissions), password_hash(password_hash) {}
};

typedef std::unordered_map<std::string, auth_data> auth_table_t;

template <class Body, class Allocator>
std::optional<AuthorizationType> get_auth(
	const http::request<Body, http::basic_fields<Allocator>>& req,
	const std::unordered_map<std::string, auth_data>& auth_table
) {
    if (req.find(http::field::authorization) == req.end()) {
        return std::nullopt;
    }

    // get the authorization field and separate the base64 encoded user-pass combination
    const std::string authorization = req.at(http::field::authorization);
    const std::string user_pass = authorization.substr(authorization.find(' ') + 1);

    // decode the base64 combination
    const std::size_t decoded_size = base64::decoded_size(user_pass.size());
    char* user_pass_decoded = new char[decoded_size];

    auto decode_result = 
        beast::detail::base64::decode(
            reinterpret_cast<void*>(user_pass_decoded), 
            user_pass.c_str(), 
            user_pass.size()
        );

    const std::string user_pass_str(user_pass_decoded, decode_result.first);

    delete[] user_pass_decoded;

    // separate the user-id and password (if the user exists in the auth_table)
    const std::size_t user_pass_delimeter_loc = user_pass_str.find(':');
    const std::string user_id = user_pass_str.substr(0, user_pass_delimeter_loc);

    if (auth_table.find(user_id) == auth_table.end()) {
        return std::nullopt;
    }

    const std::string password = user_pass_str.substr(user_pass_delimeter_loc + 1);
   
    // compare the sent password with the stored hash to validate
    const auth_data& data = auth_table.at(user_id);
    if (bcrypt::validatePassword(password, data.password_hash)) {
        return data.permissions;
    }
    else {
        return std::nullopt;
    }
}

std::optional<std::unordered_map<std::string, auth_data>>
open_auth_table_from_file(fs::path file_path);

#endif