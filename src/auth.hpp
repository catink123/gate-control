#ifndef AUTH_HPP
#define AUTH_HPP

#include "common.hpp"
#include <optional>
#include <array>
#include <utility>
#include <vector>
#include <filesystem>
#include <fstream>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/base64.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <regex>

namespace fs = std::filesystem;

enum AuthorizationType {
    Blocked = 0,
    View = 1,
    Control = 2,
};

const std::vector<std::pair<std::string, std::optional<AuthorizationType>>> endpoint_map = {
    { "/control", Control },
    { "/view", View },
    { "/", std::nullopt }
};

std::optional<AuthorizationType> get_endpoint_permissions(
    std::string_view path
);

struct auth_data {
    AuthorizationType permissions;
    std::string password;

    auth_data(
        const AuthorizationType permissions,
        std::string_view password
    ) : permissions(permissions), password(password) {}
};

typedef std::unordered_map<std::string, auth_data> auth_table_t;

std::string http_method_to_str(const http::verb& method);

std::optional<std::unordered_map<std::string, auth_data>>
open_auth_table_from_file(fs::path file_path);

const std::size_t NONCE_SIZE = 32;
std::string generate_nonce();

std::string to_hex(const std::string& input, bool uppercase = false);
std::string md5_hash(const std::string& input);

struct digest_auth {
    std::string username;
    std::string response;
    std::string nonce;
    std::string realm;
    std::string uri;
    std::optional<std::string> nc;
    std::optional<std::string> cnonce;
    std::optional<std::string> opaque;
    std::optional<std::string> qop;

    void set_field(std::string_view key, std::string_view value);
    bool is_valid() const;

    // MD5 hashing and no qop is assumed
    bool check_password(
        std::string_view password, 
        std::string_view method,
		std::string_view nonce,
        const auth_table_t& auth_table
    ) const;
};

const std::regex key_value_regex(R"(\s*([a-zA-Z]+)=(".+?"|[a-zA-Z0-9_\-+=]+),?\s*)");

std::optional<digest_auth> parse_digest_auth_field(
    const std::string& field_value
);

template <class Body, class Allocator>
std::optional<AuthorizationType> get_auth(
	const http::request<Body, http::basic_fields<Allocator>>& req,
	const std::unordered_map<std::string, auth_data>& auth_table,
    std::string_view nonce
) {
    if (req.find(http::field::authorization) == req.end()) {
        return std::nullopt;
    }

    // get the authorization field and parse it
    const std::string authorization = req.at(http::field::authorization);
    const auto digest_opt = parse_digest_auth_field(authorization);
    if (!digest_opt) {
        return std::nullopt;
    }

    const digest_auth& digest = digest_opt.value();
    if (auth_table.find(digest.username) == auth_table.end()) {
        return std::nullopt;
    }

    const auth_data& stored_auth_data = auth_table.at(digest.username);
    const std::string method = http_method_to_str(req.base().method());

    if (digest.check_password(stored_auth_data.password, method, nonce, auth_table)) {
        return stored_auth_data.permissions;
    }
    else {
        return std::nullopt;
    }

    //// get the authorization field and separate the base64 encoded user-pass combination
    //const std::string authorization = req.at(http::field::authorization);
    //const std::string user_pass = authorization.substr(authorization.find(' ') + 1);

    //// decode the base64 combination
    //const std::size_t decoded_size = base64::decoded_size(user_pass.size());
    ////char* user_pass_decoded = new char[decoded_size];
    //std::vector<char> user_pass_decoded(decoded_size, '\0');

    //auto decode_result = 
    //    beast::detail::base64::decode(
    //        reinterpret_cast<void*>(&user_pass_decoded[0]),
    //        user_pass.c_str(), 
    //        user_pass.size()
    //    );

    //const std::string user_pass_str(user_pass_decoded, decode_result.first);

    ////delete[] user_pass_decoded;

    //// separate the user-id and password (if the user exists in the auth_table)
    //const std::size_t user_pass_delimeter_loc = user_pass_str.find(':');
    //const std::string user_id = user_pass_str.substr(0, user_pass_delimeter_loc);

    //if (auth_table.find(user_id) == auth_table.end()) {
    //    return std::nullopt;
    //}

    //const std::string password = user_pass_str.substr(user_pass_delimeter_loc + 1);
   
    //// compare the sent password with the stored hash to validate
    //const auth_data& data = auth_table.at(user_id);
    //if (bcrypt::validatePassword(password, data.password)) {
    //    return data.permissions;
    //}
    //else {
    //    return std::nullopt;
    //}
}

#endif