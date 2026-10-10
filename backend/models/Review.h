#pragma once

#include <string>
#include <json/json.h>

namespace mathan_mart {

struct Review {
    int id = 0;
    int buyerId = 0;
    std::string buyerName;
    int productId = 0;
    int rating = 5;
    std::string comment;
    std::string createdAt;

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["buyerId"] = buyerId;
        val["buyerName"] = buyerName;
        val["productId"] = productId;
        val["rating"] = rating;
        val["comment"] = comment;
        val["createdAt"] = createdAt;
        return val;
    }
};

} // namespace mathan_mart
