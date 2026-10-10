#include "CartController.h"
#include "../services/OrderService.h"
#include "../middleware/AuthMiddleware.h"

namespace mathan_mart {

namespace {

void addCors(const drogon::HttpResponsePtr& resp) {
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Auth-Token");
}

} // anonymous namespace

void CartController::getCart(const drogon::HttpRequestPtr& req, 
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireRole(req, "buyer", callback, session)) {
        return;
    }

    double totalAmount = 0.0;
    auto items = OrderService::getInstance().getCart(session.userId, totalAmount);

    Json::Value itemsArr(Json::arrayValue);
    int totalCount = 0;
    for (const auto& item : items) {
        itemsArr.append(item.toJson());
        totalCount += item.quantity;
    }

    Json::Value data;
    data["items"] = itemsArr;
    data["totalAmount"] = totalAmount;
    data["itemCount"] = totalCount;

    Json::Value res;
    res["success"] = true;
    res["data"] = data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

void CartController::addToCart(const drogon::HttpRequestPtr& req, 
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireRole(req, "buyer", callback, session)) {
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        AuthMiddleware::sendJsonError(callback, 400, "Invalid JSON payload.");
        return;
    }

    int productId = json->get("productId", 0).asInt();
    if (productId <= 0) {
        productId = json->get("product_id", 0).asInt();
    }
    int quantity = json->get("quantity", 1).asInt();

    if (productId <= 0) {
        AuthMiddleware::sendJsonError(callback, 400, "Valid productId is required.");
        return;
    }

    std::string errorMsg;
    int statusCode = 200;
    bool ok = OrderService::getInstance().addToCart(session.userId, productId, quantity, errorMsg, statusCode);

    Json::Value res;
    res["success"] = ok;
    res["message"] = ok ? "Item added to cart." : errorMsg;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(ok ? 200 : statusCode));
    addCors(resp);
    callback(resp);
}

void CartController::updateCartItem(const drogon::HttpRequestPtr& req, 
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback, 
                                    int productId) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireRole(req, "buyer", callback, session)) {
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        AuthMiddleware::sendJsonError(callback, 400, "Invalid JSON payload.");
        return;
    }

    int quantity = json->get("quantity", 1).asInt();

    std::string errorMsg;
    int statusCode = 200;
    bool ok = OrderService::getInstance().updateCartItem(session.userId, productId, quantity, errorMsg, statusCode);

    Json::Value res;
    res["success"] = ok;
    res["message"] = ok ? "Cart item updated." : errorMsg;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(ok ? 200 : statusCode));
    addCors(resp);
    callback(resp);
}

void CartController::deleteCartItem(const drogon::HttpRequestPtr& req, 
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback, 
                                    int productId) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireRole(req, "buyer", callback, session)) {
        return;
    }

    bool ok = OrderService::getInstance().removeCartItem(session.userId, productId);

    Json::Value res;
    res["success"] = ok;
    res["message"] = ok ? "Item removed from cart." : "Item not found in cart.";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(ok ? 200 : 404));
    addCors(resp);
    callback(resp);
}

void CartController::clearCart(const drogon::HttpRequestPtr& req, 
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireRole(req, "buyer", callback, session)) {
        return;
    }

    bool ok = OrderService::getInstance().clearCart(session.userId);

    Json::Value res;
    res["success"] = ok;
    res["message"] = ok ? "Cart cleared successfully." : "Failed to clear cart.";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

} // namespace mathan_mart
