#pragma once

#include <string>
#include <vector>
#include <optional>
#include <json/json.h>
#include "../models/CartItem.h"
#include "../models/Order.h"
#include "../models/OrderItem.h"
#include "../models/Review.h"

namespace mathan_mart {

class OrderService {
public:
    static OrderService& getInstance();

    OrderService(const OrderService&) = delete;
    OrderService& operator=(const OrderService&) = delete;

    // Cart management
    std::vector<CartItem> getCart(int buyerId, double& outTotal);
    bool addToCart(int buyerId, int productId, int quantity, std::string& errorMsg, int& statusCode);
    bool updateCartItem(int buyerId, int productId, int quantity, std::string& errorMsg, int& statusCode);
    bool removeCartItem(int buyerId, int productId);
    bool clearCart(int buyerId);

    // Checkout & Orders
    struct CheckoutResult {
        bool success = false;
        std::string message;
        int statusCode = 200;
        int orderId = 0;
        double totalAmount = 0.0;
        std::vector<OrderItem> items;
    };

    CheckoutResult checkout(int buyerId);

    std::vector<Order> getBuyerOrders(int buyerId);
    std::optional<Order> getOrderById(int orderId, int userId, const std::string& userRole);
    std::vector<OrderItem> getSellerOrderItems(int sellerId);
    std::vector<Order> getAllOrders();
    bool updateOrderStatus(int orderId, const std::string& status);

    // Reviews
    bool addReview(int buyerId, int productId, int rating, const std::string& comment, std::string& errorMsg, int& statusCode);
    std::vector<Review> getProductReviews(int productId);

    // Admin Statistics
    Json::Value getAdminStats();

private:
    OrderService() = default;
};

} // namespace mathan_mart
