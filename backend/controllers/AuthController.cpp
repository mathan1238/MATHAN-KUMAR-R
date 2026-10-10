#include "AuthController.h"
#include "../services/AuthService.h"
#include "../middleware/AuthMiddleware.h"

namespace mathan_mart {

namespace {

void addCors(const drogon::HttpResponsePtr& resp) {
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Auth-Token");
}

} // anonymous namespace

void AuthController::registerUser(const drogon::HttpRequestPtr& req, 
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        AuthMiddleware::sendJsonError(callback, 400, "Invalid JSON payload.");
        return;
    }

    std::string name = json->get("name", "").asString();
    std::string email = json->get("email", "").asString();
    std::string password = json->get("password", "").asString();
    std::string role = json->get("role", "buyer").asString();

    auto result = AuthService::getInstance().registerUser(name, email, password, role);

    Json::Value respJson;
    respJson["success"] = result.success;
    respJson["message"] = result.message;

    if (result.success) {
        Json::Value data;
        data["token"] = result.token;
        data["user"] = result.user.toJson();
        respJson["data"] = data;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(respJson);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.statusCode));
    addCors(resp);
    callback(resp);
}

void AuthController::loginUser(const drogon::HttpRequestPtr& req, 
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        AuthMiddleware::sendJsonError(callback, 400, "Invalid JSON payload.");
        return;
    }

    std::string email = json->get("email", "").asString();
    std::string password = json->get("password", "").asString();

    auto result = AuthService::getInstance().loginUser(email, password);

    Json::Value respJson;
    respJson["success"] = result.success;
    respJson["message"] = result.message;

    if (result.success) {
        Json::Value data;
        data["token"] = result.token;
        data["user"] = result.user.toJson();
        respJson["data"] = data;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(respJson);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.statusCode));
    addCors(resp);
    callback(resp);
}

void AuthController::logoutUser(const drogon::HttpRequestPtr& req, 
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    if (req->method() == drogon::Options) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        addCors(resp);
        callback(resp);
        return;
    }

    std::string token = AuthMiddleware::extractToken(req);
    if (!token.empty()) {
        AuthService::getInstance().logout(token);
    }

    Json::Value respJson;
    respJson["success"] = true;
    respJson["message"] = "Logged out successfully.";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(respJson);
    addCors(resp);
    callback(resp);
}

void AuthController::getCurrentUser(const drogon::HttpRequestPtr& req, 
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
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

    auto userOpt = AuthService::getInstance().getUserById(session.userId);
    if (!userOpt.has_value()) {
        AuthMiddleware::sendJsonError(callback, 404, "User account no longer exists.");
        return;
    }

    Json::Value respJson;
    respJson["success"] = true;
    respJson["data"] = userOpt->toJson();

    auto resp = drogon::HttpResponse::newHttpJsonResponse(respJson);
    addCors(resp);
    callback(resp);
}

} // namespace mathan_mart
