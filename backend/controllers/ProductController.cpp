#include "ProductController.h"
#include "../services/ProductService.h"
#include "../middleware/AuthMiddleware.h"

namespace mathan_mart {

namespace {

void addCors(const drogon::HttpResponsePtr& resp) {
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Auth-Token");
}

} // anonymous namespace

void ProductController::getProducts(const drogon::HttpRequestPtr& req, 
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    std::string search = req->getParameter("search");
    std::string category = req->getParameter("category");

    auto products = ProductService::getInstance().getProducts(search, category);

    Json::Value arr(Json::arrayValue);
    for (const auto& p : products) {
        arr.append(p.toJson());
    }

    Json::Value res;
    res["success"] = true;
    res["data"] = arr;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

void ProductController::getProductById(const drogon::HttpRequestPtr& req, 
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback, 
                                       int id) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    auto productOpt = ProductService::getInstance().getProductById(id);
    if (!productOpt.has_value()) {
        AuthMiddleware::sendJsonError(callback, 404, "Product not found");
        return;
    }

    Json::Value res;
    res["success"] = true;
    res["data"] = productOpt->toJson();

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

void ProductController::createProduct(const drogon::HttpRequestPtr& req, 
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireRole(req, "seller", callback, session)) {
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        AuthMiddleware::sendJsonError(callback, 400, "Invalid JSON payload.");
        return;
    }

    std::string name = json->get("name", "").asString();
    std::string description = json->get("description", "").asString();
    double price = json->get("price", 0.0).asDouble();
    std::string category = json->get("category", "").asString();
    int stock = json->get("stock", 0).asInt();
    std::string imageUrl = json->get("imageUrl", "").asString();
    if (imageUrl.empty()) {
        imageUrl = json->get("image_url", "").asString();
    }

    auto result = ProductService::getInstance().createProduct(session.userId, name, description, 
                                                              price, category, stock, imageUrl);

    Json::Value res;
    res["success"] = result.success;
    res["message"] = result.message;
    if (result.success) {
        res["data"] = result.product.toJson();
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.statusCode));
    addCors(resp);
    callback(resp);
}

void ProductController::updateProduct(const drogon::HttpRequestPtr& req, 
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback, 
                                      int id) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireAnyRole(req, {"seller", "admin"}, callback, session)) {
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        AuthMiddleware::sendJsonError(callback, 400, "Invalid JSON payload.");
        return;
    }

    std::string name = json->get("name", "").asString();
    std::string description = json->get("description", "").asString();
    double price = json->get("price", 0.0).asDouble();
    std::string category = json->get("category", "").asString();
    int stock = json->get("stock", 0).asInt();
    std::string imageUrl = json->get("imageUrl", "").asString();
    if (imageUrl.empty()) {
        imageUrl = json->get("image_url", "").asString();
    }

    auto result = ProductService::getInstance().updateProduct(id, session.userId, session.role, 
                                                              name, description, price, category, stock, imageUrl);

    Json::Value res;
    res["success"] = result.success;
    res["message"] = result.message;
    if (result.success) {
        res["data"] = result.product.toJson();
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.statusCode));
    addCors(resp);
    callback(resp);
}

void ProductController::deleteProduct(const drogon::HttpRequestPtr& req, 
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback, 
                                      int id) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireAnyRole(req, {"seller", "admin"}, callback, session)) {
        return;
    }

    auto result = ProductService::getInstance().deleteProduct(id, session.userId, session.role);

    Json::Value res;
    res["success"] = result.success;
    res["message"] = result.message;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.statusCode));
    addCors(resp);
    callback(resp);
}

void ProductController::getMyProducts(const drogon::HttpRequestPtr& req, 
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireRole(req, "seller", callback, session)) {
        return;
    }

    auto products = ProductService::getInstance().getProducts("", "", session.userId);

    Json::Value arr(Json::arrayValue);
    for (const auto& p : products) {
        arr.append(p.toJson());
    }

    Json::Value res;
    res["success"] = true;
    res["data"] = arr;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

} // namespace mathan_mart
