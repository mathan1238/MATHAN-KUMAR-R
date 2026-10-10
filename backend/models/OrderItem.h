#pragma once

#include <string>
#include <json/json.h>

namespace mathan_mart {

struct OrderItem {
    int id = 0;
    int orderId = 0;
    int productId = 0;
    int sellerId = 0;
    int quantity = 0;
    double price = 0.0;
    std::string productName;
    std::string productImageUrl;
    std::string sellerName;

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["orderId"] = orderId;
        val["productId"] = productId;
        val["sellerId"] = sellerId;
        val["quantity"] = quantity;
        val["price"] = price;
        val["productName"] = productName;
        val["productImageUrl"] = productImageUrl;
        val["sellerName"] = sellerName;
        val["subtotal"] = quantity * price;
        return val;
    }
};

} // namespace mathan_mart
