#pragma once

#include <drogon/HttpController.h>

namespace mathan_mart {

class ProductController : public drogon::HttpController<ProductController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProductController::getProducts, "/api/products", drogon::Get, drogon::Options);
    ADD_METHOD_TO(ProductController::getProductById, "/api/products/{id}", drogon::Get, drogon::Options);
    ADD_METHOD_TO(ProductController::createProduct, "/api/products", drogon::Post, drogon::Options);
    ADD_METHOD_TO(ProductController::updateProduct, "/api/products/{id}", drogon::Put, drogon::Options);
    ADD_METHOD_TO(ProductController::deleteProduct, "/api/products/{id}", drogon::Delete, drogon::Options);
    ADD_METHOD_TO(ProductController::getMyProducts, "/api/seller/my-products", drogon::Get, drogon::Options);
    METHOD_LIST_END

    void getProducts(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getProductById(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int id);
    void createProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void updateProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int id);
    void deleteProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int id);
    void getMyProducts(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace mathan_mart
