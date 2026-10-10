#pragma once

#include <string>
#include <json/json.h>

namespace mathan_mart {

struct User {
    int id = 0;
    std::string name;
    std::string email;
    std::string passwordHash;
    std::string role; // "buyer", "seller", "admin"
    std::string createdAt;

    Json::Value toJson(bool includeHash = false) const {
        Json::Value val;
        val["id"] = id;
        val["name"] = name;
        val["email"] = email;
        val["role"] = role;
        val["createdAt"] = createdAt;
        if (includeHash) {
            val["passwordHash"] = passwordHash;
        }
        return val;
    }
};

} // namespace mathan_mart
