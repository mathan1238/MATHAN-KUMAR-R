#pragma once

#include <string>
#include <json/json.h>

namespace mathan_mart {

struct CartItem {
    int id = 0;
    int buyerId = 0;
    int productId = 0;
    int quantity = 0;
    std::string productName;
    double productPrice = 0.0;
    std::string productCategory;
    std::string productImageUrl;
    int productStock = 0;
    double subtotal = 0.0;

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["buyerId"] = buyerId;
        val["productId"] = productId;
        val["quantity"] = quantity;
        val["name"] = productName;
        val["price"] = productPrice;
        val["category"] = productCategory;
        val["imageUrl"] = productImageUrl;
        val["stock"] = productStock;
        val["subtotal"] = subtotal;
        return val;
    }
};

} // namespace mathan_mart
