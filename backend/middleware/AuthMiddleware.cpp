#include "AuthMiddleware.h"

namespace mathan_mart {

std::string AuthMiddleware::extractToken(const drogon::HttpRequestPtr& req) {
    std::string authHeader = req->getHeader("Authorization");
    if (authHeader.empty()) {
        authHeader = req->getHeader("authorization");
    }

    if (!authHeader.empty()) {
        if (authHeader.rfind("Bearer ", 0) == 0 || authHeader.rfind("bearer ", 0) == 0) {
            return authHeader.substr(7);
        }
        return authHeader;
    }

    std::string xToken = req->getHeader("X-Auth-Token");
    if (!xToken.empty()) return xToken;

    std::string cookieToken = req->getCookie("mathan_mart_token");
    if (!cookieToken.empty()) return cookieToken;

    return "";
}

std::optional<SessionInfo> AuthMiddleware::authenticate(const drogon::HttpRequestPtr& req) {
    std::string token = extractToken(req);
    if (token.empty()) return std::nullopt;
    return AuthService::getInstance().validateToken(token);
}

void AuthMiddleware::sendJsonError(std::function<void(const drogon::HttpResponsePtr&)>& callback,
                                   int httpStatusCode,
                                   const std::string& message) {
    Json::Value err;
    err["success"] = false;
    err["message"] = message;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(httpStatusCode));
    callback(resp);
}

bool AuthMiddleware::requireAuth(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>& callback,
                                 SessionInfo& outSession) {
    auto sess = authenticate(req);
    if (!sess.has_value()) {
        sendJsonError(callback, 401, "Authentication required. Please log in.");
        return false;
    }
    outSession = sess.value();
    return true;
}

bool AuthMiddleware::requireRole(const drogon::HttpRequestPtr& req,
                                 const std::string& requiredRole,
                                 std::function<void(const drogon::HttpResponsePtr&)>& callback,
                                 SessionInfo& outSession) {
    if (!requireAuth(req, callback, outSession)) {
        return false;
    }

    if (outSession.role != requiredRole) {
        sendJsonError(callback, 403, "Access denied. Requires '" + requiredRole + "' role.");
        return false;
    }

    return true;
}

bool AuthMiddleware::requireAnyRole(const drogon::HttpRequestPtr& req,
                                    const std::vector<std::string>& allowedRoles,
                                    std::function<void(const drogon::HttpResponsePtr&)>& callback,
                                    SessionInfo& outSession) {
    if (!requireAuth(req, callback, outSession)) {
        return false;
    }

    bool matched = false;
    for (const auto& r : allowedRoles) {
        if (outSession.role == r) {
            matched = true;
            break;
        }
    }

    if (!matched) {
        sendJsonError(callback, 403, "Access denied. Insufficient permissions.");
        return false;
    }

    return true;
}

} // namespace mathan_mart
