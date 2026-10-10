#pragma once

#include <drogon/HttpController.h>

namespace mathan_mart {

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/api/auth/register", drogon::Post, drogon::Options);
    ADD_METHOD_TO(AuthController::loginUser, "/api/auth/login", drogon::Post, drogon::Options);
    ADD_METHOD_TO(AuthController::logoutUser, "/api/auth/logout", drogon::Post, drogon::Options);
    ADD_METHOD_TO(AuthController::getCurrentUser, "/api/auth/me", drogon::Get, drogon::Options);
    METHOD_LIST_END

    void registerUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void loginUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void logoutUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getCurrentUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace mathan_mart
