#ifndef AUTH_HPP
#define AUTH_HPP

#include "common.hpp"
#include <unordered_map>
#include <optional>
#include <bcrypt.h>
#include <boost/beast/core/detail/base64.hpp>
#include <array>

namespace base64 = beast::detail::base64;

enum AuthorizationType {
    Control,
    View
};

struct auth_data {
    AuthorizationType permissions;
    std::string password_hash;

    auth_data(
        const AuthorizationType permissions,
        std::string_view password_hash
    ) : permissions(permissions), password_hash(password_hash) {}
};

template <class Body, class Allocator>
std::optional<AuthorizationType> check_auth(
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

const std::array<std::string, 3> unauthable_resources = { "/", "/index.html", "/favicon.ico" };

template <class Body, class Allocator>
bool requires_auth(
    const http::request<Body, http::basic_fields<Allocator>>& req
) {
    return std::find(
        unauthable_resources.begin(), 
        unauthable_resources.end(), 
        req.target()
    ) == unauthable_resources.end();
}

#endif