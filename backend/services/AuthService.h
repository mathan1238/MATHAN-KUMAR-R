#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <json/json.h>
#include "../models/User.h"

namespace mathan_mart {

struct SessionInfo {
    int userId = 0;
    std::string name;
    std::string email;
    std::string role;
    std::chrono::system_clock::time_point expiresAt;
};

class AuthService {
public:
    static AuthService& getInstance();

    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;

    // Password hashing & verification
    static std::string hashPassword(const std::string& password, const std::string& salt = "");
    static bool verifyPassword(const std::string& password, const std::string& storedHash);

    // Registration & Login
    struct AuthResult {
        bool success = false;
        std::string message;
        std::string token;
        User user;
        int statusCode = 200;
    };

    AuthResult registerUser(const std::string& name, const std::string& email, const std::string& password, const std::string& role);
    AuthResult loginUser(const std::string& email, const std::string& password);
    void logout(const std::string& token);

    // Session / Token validation
    std::optional<SessionInfo> validateToken(const std::string& token);
    std::optional<User> getUserById(int userId);

private:
    AuthService() = default;

    std::string generateToken();
    std::unordered_map<std::string, SessionInfo> activeSessions_;
    std::mutex sessionMutex_;
};

} // namespace mathan_mart
