#include "ReviewController.h"
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

void ReviewController::createReview(const drogon::HttpRequestPtr& req, 
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

    int rating = json->get("rating", 5).asInt();
    std::string comment = json->get("comment", "").asString();

    std::string errorMsg;
    int statusCode = 200;
    bool ok = OrderService::getInstance().addReview(session.userId, productId, rating, comment, errorMsg, statusCode);

    Json::Value res;
    res["success"] = ok;
    res["message"] = ok ? "Review submitted successfully!" : errorMsg;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(ok ? 201 : statusCode));
    addCors(resp);
    callback(resp);
}

void ReviewController::getReviews(const drogon::HttpRequestPtr& req, 
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback, 
                                  int productId) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    auto reviews = OrderService::getInstance().getProductReviews(productId);

    Json::Value arr(Json::arrayValue);
    for (const auto& r : reviews) {
        arr.append(r.toJson());
    }

    Json::Value res;
    res["success"] = true;
    res["data"] = arr;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

} // namespace mathan_mart
