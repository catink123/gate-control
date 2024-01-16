#ifndef AUTH_HPP
#define AUTH_HPP

#include "common.hpp"
#include <unordered_map>
#include <optional>
#include <bcrypt.h>
#include <boost/beast/core/detail/base64.hpp>
#include <array>
#include <unordered_map>

namespace base64 = beast::detail::base64;

enum AuthorizationType {
    Control,
    View,
    Blocked
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

// catink123:testpassword123
// guest:guest
const std::unordered_map<std::string, auth_data> temp_auth_table = {
    { "catink123", auth_data(Control, "$2a$10$o12u27uUOjD6rJ0dlEE/EuL8EqGa7y8iwZqAp3wF0WBS4.Vu/9jhK") },
    { "guest", auth_data(View, "$2a$10$vYQHg8mBFTle1OzRp31MsOMvrmfQ52xfHUGFoi3aTe6Vp8GhDRzBy") }
};

template <class Body, class Allocator>
std::optional<AuthorizationType> get_auth(
	const http::request<Body, http::basic_fields<Allocator>>& req,
	const std::unordered_map<std::string, auth_data>& auth_table
) {
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

#endif