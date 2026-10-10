#include "OrderController.h"
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

void OrderController::checkout(const drogon::HttpRequestPtr& req, 
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

    auto result = OrderService::getInstance().checkout(session.userId);

    Json::Value res;
    res["success"] = result.success;
    res["message"] = result.message;

    if (result.success) {
        Json::Value data;
        data["orderId"] = result.orderId;
        data["totalAmount"] = result.totalAmount;
        Json::Value itemsArr(Json::arrayValue);
        for (const auto& item : result.items) {
            itemsArr.append(item.toJson());
        }
        data["items"] = itemsArr;
        res["data"] = data;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.statusCode));
    addCors(resp);
    callback(resp);
}

void OrderController::getBuyerOrders(const drogon::HttpRequestPtr& req, 
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

    auto orders = OrderService::getInstance().getBuyerOrders(session.userId);

    Json::Value ordersArr(Json::arrayValue);
    for (const auto& o : orders) {
        ordersArr.append(o.toJson());
    }

    Json::Value res;
    res["success"] = true;
    res["data"] = ordersArr;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

void OrderController::getOrderById(const drogon::HttpRequestPtr& req, 
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback, 
                                   int id) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireAuth(req, callback, session)) {
        return;
    }

    auto orderOpt = OrderService::getInstance().getOrderById(id, session.userId, session.role);
    if (!orderOpt.has_value()) {
        AuthMiddleware::sendJsonError(callback, 404, "Order not found or access denied.");
        return;
    }

    Json::Value res;
    res["success"] = true;
    res["data"] = orderOpt->toJson();

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

void OrderController::getSellerOrders(const drogon::HttpRequestPtr& req, 
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

    auto items = OrderService::getInstance().getSellerOrderItems(session.userId);

    Json::Value itemsArr(Json::arrayValue);
    for (const auto& item : items) {
        itemsArr.append(item.toJson());
    }

    Json::Value res;
    res["success"] = true;
    res["data"] = itemsArr;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

void OrderController::getAdminOrders(const drogon::HttpRequestPtr& req, 
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    SessionInfo session;
    if (!AuthMiddleware::requireRole(req, "admin", callback, session)) {
        return;
    }

    auto orders = OrderService::getInstance().getAllOrders();

    Json::Value ordersArr(Json::arrayValue);
    for (const auto& o : orders) {
        ordersArr.append(o.toJson());
    }

    Json::Value res;
    res["success"] = true;
    res["data"] = ordersArr;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

void OrderController::updateStatus(const drogon::HttpRequestPtr& req, 
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

    std::string status = json->get("status", "Pending").asString();
    bool ok = OrderService::getInstance().updateOrderStatus(id, status);

    Json::Value res;
    res["success"] = ok;
    res["message"] = ok ? "Order status updated to " + status : "Failed to update order status.";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

} // namespace mathan_mart
