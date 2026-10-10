#pragma once

#include <drogon/HttpController.h>

namespace mathan_mart {

class AdminController : public drogon::HttpController<AdminController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminController::getUsers, "/api/admin/users", drogon::Get, drogon::Options);
    ADD_METHOD_TO(AdminController::getProducts, "/api/admin/products", drogon::Get, drogon::Options);
    ADD_METHOD_TO(AdminController::getOrders, "/api/admin/orders", drogon::Get, drogon::Options);
    ADD_METHOD_TO(AdminController::deleteProduct, "/api/admin/products/{id}", drogon::Delete, drogon::Options);
    ADD_METHOD_TO(AdminController::getStats, "/api/admin/stats", drogon::Get, drogon::Options);
    METHOD_LIST_END

    void getUsers(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getProducts(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void deleteProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int id);
    void getStats(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace mathan_mart
