#pragma once

#include <string>
#include <json/json.h>

namespace mathan_mart {

struct Product {
    int id = 0;
    int sellerId = 0;
    std::string sellerName;
    std::string name;
    std::string description;
    double price = 0.0;
    std::string category;
    int stock = 0;
    std::string imageUrl;
    std::string createdAt;
    double averageRating = 0.0;
    int reviewCount = 0;

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["sellerId"] = sellerId;
        val["sellerName"] = sellerName;
        val["name"] = name;
        val["description"] = description;
        val["price"] = price;
        val["category"] = category;
        val["stock"] = stock;
        val["imageUrl"] = imageUrl;
        val["createdAt"] = createdAt;
        val["averageRating"] = averageRating;
        val["reviewCount"] = reviewCount;
        return val;
    }
};

} // namespace mathan_mart
