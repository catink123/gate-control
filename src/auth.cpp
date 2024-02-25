#include "auth.hpp"

std::optional<AuthorizationType> get_endpoint_permissions(
	std::string_view path
) {
    for (const auto& pair : endpoint_map) {
        const std::string& endpoint = pair.first;
        const auto& permissions = pair.second;

        if (path.starts_with(endpoint)) {
            if (path.size() > endpoint.size()) {
                if (path.find("/", endpoint.size()) != std::string_view::npos) {
					return get_endpoint_permissions(path.substr(0, path.rfind('/')));
                }
                else {
                    return std::nullopt;
                }
            }
            return permissions;
        }
    }

    return std::nullopt;
}

std::optional<std::unordered_map<std::string, auth_data>> 
open_auth_table_from_file(fs::path file_path) {
    std::ifstream file;
    file.open(file_path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::unordered_map<std::string, auth_data> auth_table;
    std::string current_line;

    // parsing line by line with the following format:
    // login:permissions:password
    while (std::getline(file, current_line)) {
        std::size_t first_delimeter = current_line.find(':');
        if (first_delimeter == std::string::npos) {
            continue;
        }

        std::size_t second_delimeter = current_line.find(':', first_delimeter + 1);
        if (second_delimeter == std::string::npos || current_line.size() == second_delimeter) {
            continue;
        }

        std::string login = current_line.substr(0, first_delimeter);
        std::string permissions_str = 
            current_line.substr(
                first_delimeter + 1, 
                second_delimeter - first_delimeter - 1
            );
        unsigned int permissions = 99;
        try {
            permissions = std::stoi(permissions_str);
        }
        catch (...) {
            continue;
        }
        if (permissions > 2) {
            continue;
        }

        std::string password = current_line.substr(second_delimeter + 1);

        auth_table.insert({
            login,
            auth_data(
                static_cast<AuthorizationType>(permissions),
                password
            ) 
		});
    }

    file.close();

    return auth_table;
}

void digest_auth::set_field(
    std::string_view key,
    std::string_view value
) {
    if (key == "username")  username = value;
    if (key == "response")  response = value;
    if (key == "nonce")     nonce = value;
    if (key == "realm")     realm = value;
    if (key == "uri")       uri = value;
    if (key == "nc")        nc = value;
    if (key == "cnonce")    cnonce = value;
    if (key == "opaque")    opaque = value;
    if (key == "qop")       qop = value;
}

bool digest_auth::is_valid() const {
    if (
        username.size() != 0 &&
        response.size() != 0 &&
        nonce.size() != 0 &&
        realm.size() != 0 &&
        uri.size() != 0
	) {
        if (qop) {
            return cnonce && nc;
        }
		return !cnonce && !nc;
    }
    return false;
}

std::string to_hex(const std::string& input, bool uppercase) {
    std::string output;

    CryptoPP::StringSource(input, true,
        new CryptoPP::HexEncoder(
            new CryptoPP::StringSink(output),
            uppercase
        )
    );

    return output;
}

std::string sha256_hash(const std::string& input) {
    std::string output;

    CryptoPP::SHA256 hash;

    CryptoPP::StringSource(input, true,
        new CryptoPP::HashFilter(hash,
            new CryptoPP::HexEncoder(
				new CryptoPP::StringSink(output),
                false
            )
        )
	);

    return output;

  //  CryptoPP::Weak::MD5 hash;

  //  hash.Update(
  //      reinterpret_cast<const CryptoPP::byte*>(
  //          input.data()
		//),
  //      input.size()
  //  );

  //  unsigned int size = hash.DigestSize();
  //  std::vector<char> hashed_value(size, '\0');
  //  char* hashed_value_c_str = &hashed_value[0];

  //  hash.Final(
  //      reinterpret_cast<CryptoPP::byte*>(hashed_value_c_str)
  //  );

  //  return std::string(hashed_value_c_str, size);
}

std::string http_method_to_str(const http::verb& method) {
    if (method == http::verb::get)          return "GET";
    if (method == http::verb::put)          return "PUT";
    if (method == http::verb::delete_)      return "DELETE";
    if (method == http::verb::head)         return "HEAD";
    if (method == http::verb::options)      return "OPTIONS";
    if (method == http::verb::connect)      return "CONNECT";
    return "unknown";
}

bool digest_auth::check_password(
    std::string_view password,
    std::string_view method,
    std::string_view nonce,
    std::string_view opaque,
    const auth_table_t& auth_table
) const {
    if (
        // there's no user with the specified name
        auth_table.find(username) == auth_table.end() ||
        // no qop parameter
        !qop || 
        // no opaque parameter
        !this->opaque ||
        // specified opaque isn't the same as stored opaque
        this->opaque != opaque
	) {
        return false;
    }

    const auto& data = auth_table.at(username);

    std::string A1 = 
        username + ':' + realm + ':' + std::string(password);
    std::string A1_hash = sha256_hash(A1);

    std::string A2 = std::string(method) + ':' + uri;
	std::string A2_hash = sha256_hash(A2);

    std::string KD_unhashed =
        A1_hash + ':' +
        std::string(nonce) + ':' +
        nc.value() + ':' +
        cnonce.value() + ':' +
        qop.value() + ':' +
        A2_hash;
    std::string KD = sha256_hash(KD_unhashed);

    return response == KD;
}

std::optional<digest_auth> parse_digest_auth_field(
    const std::string& field_value
) {
    // make sure the field_value starts with the word Digest
    if (!field_value.starts_with("Digest")) {
        return std::nullopt;
    }

    digest_auth params;

    auto it = std::sregex_iterator(
        field_value.begin(),
        field_value.end(),
        key_value_regex
    );
    
    for (; it != std::sregex_iterator(); it++) {
        auto smatch = *it;

        // make sure there are 3 submatches for the whole match, 
        // the key and the value
        if (std::distance(smatch.begin(), smatch.end()) != 3) {
            return std::nullopt;
        }

        auto key = smatch[1].str();
        auto value = smatch[2].str();

        if (value[0] == '"' && value[value.size() - 1] == '"') {
            value = value.substr(1, value.size() - 2);
        }

        params.set_field(key, value);
    }

    if (params.is_valid()) {
        return params;
    }
    else {
        return std::nullopt;
    }
}

std::string generate_base64_str(unsigned int size) {
    std::string output;

    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::RandomNumberSource _(rng, size, true,
        new CryptoPP::Base64URLEncoder(
            new CryptoPP::StringSink(output)
        )
	);

    return output;
}

std::string generate_digest_response(
    const std::string& nonce, 
    const std::string& opaque,
    bool stale
) {
    std::string result = 
        R"(Digest )"
        R"(realm="viewcontrol", )"
        R"(nonce=")" + nonce + R"(", )"
        R"(algorithm=SHA-256, )"
        R"(qop="auth", )"
        R"(opaque=")" + opaque + '"';

    if (stale) {
        result += ", stale=TRUE";
    }

    return result;
}
