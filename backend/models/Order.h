#pragma once

#include <string>
#include <vector>
#include <json/json.h>
#include "OrderItem.h"

namespace mathan_mart {

struct Order {
    int id = 0;
    int buyerId = 0;
    std::string buyerName;
    std::string buyerEmail;
    double totalAmount = 0.0;
    std::string status = "Pending"; // Pending, Processing, Shipped, Delivered, Cancelled
    std::string createdAt;
    std::vector<OrderItem> items;

    Json::Value toJson() const {
        Json::Value val;
        val["id"] = id;
        val["buyerId"] = buyerId;
        val["buyerName"] = buyerName;
        val["buyerEmail"] = buyerEmail;
        val["totalAmount"] = totalAmount;
        val["status"] = status;
        val["createdAt"] = createdAt;

        Json::Value itemsArray(Json::arrayValue);
        for (const auto& item : items) {
            itemsArray.append(item.toJson());
        }
        val["items"] = itemsArray;
        return val;
    }
};

} // namespace mathan_mart
