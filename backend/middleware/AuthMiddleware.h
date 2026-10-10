#pragma once

#include <drogon/drogon.h>
#include <optional>
#include <vector>
#include <string>
#include "../services/AuthService.h"

namespace mathan_mart {

class AuthMiddleware {
public:
    static std::string extractToken(const drogon::HttpRequestPtr& req);

    static std::optional<SessionInfo> authenticate(const drogon::HttpRequestPtr& req);

    static bool requireAuth(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>& callback,
                           SessionInfo& outSession);

    static bool requireRole(const drogon::HttpRequestPtr& req,
                           const std::string& requiredRole,
                           std::function<void(const drogon::HttpResponsePtr&)>& callback,
                           SessionInfo& outSession);

    static bool requireAnyRole(const drogon::HttpRequestPtr& req,
                              const std::vector<std::string>& allowedRoles,
                              std::function<void(const drogon::HttpResponsePtr&)>& callback,
                              SessionInfo& outSession);

    static void sendJsonError(std::function<void(const drogon::HttpResponsePtr&)>& callback,
                             int httpStatusCode,
                             const std::string& message);
};

} // namespace mathan_mart
