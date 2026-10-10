#pragma once

#include <drogon/HttpController.h>

namespace mathan_mart {

class CartController : public drogon::HttpController<CartController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CartController::getCart, "/api/cart", drogon::Get, drogon::Options);
    ADD_METHOD_TO(CartController::addToCart, "/api/cart", drogon::Post, drogon::Options);
    ADD_METHOD_TO(CartController::updateCartItem, "/api/cart/{productId}", drogon::Put, drogon::Options);
    ADD_METHOD_TO(CartController::deleteCartItem, "/api/cart/{productId}", drogon::Delete, drogon::Options);
    ADD_METHOD_TO(CartController::clearCart, "/api/cart", drogon::Delete, drogon::Options);
    METHOD_LIST_END

    void getCart(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void addToCart(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void updateCartItem(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId);
    void deleteCartItem(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId);
    void clearCart(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace mathan_mart
