#pragma once

#include <drogon/HttpController.h>

namespace mathan_mart {

class OrderController : public drogon::HttpController<OrderController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(OrderController::checkout, "/api/orders/checkout", drogon::Post, drogon::Options);
    ADD_METHOD_TO(OrderController::getBuyerOrders, "/api/orders", drogon::Get, drogon::Options);
    ADD_METHOD_TO(OrderController::getOrderById, "/api/orders/{id}", drogon::Get, drogon::Options);
    ADD_METHOD_TO(OrderController::getSellerOrders, "/api/seller/orders", drogon::Get, drogon::Options);
    ADD_METHOD_TO(OrderController::getAdminOrders, "/api/admin/orders", drogon::Get, drogon::Options);
    ADD_METHOD_TO(OrderController::updateStatus, "/api/orders/{id}/status", drogon::Put, drogon::Options);
    METHOD_LIST_END

    void checkout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getBuyerOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getOrderById(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int id);
    void getSellerOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getAdminOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void updateStatus(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int id);
};

} // namespace mathan_mart
