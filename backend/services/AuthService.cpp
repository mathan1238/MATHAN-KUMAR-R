#include "AuthService.h"
#include "Database.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <regex>

namespace mathan_mart {

namespace {

std::string bytesToHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return ss.str();
}

} // anonymous namespace

AuthService& AuthService::getInstance() {
    static AuthService instance;
    return instance;
}

std::string AuthService::hashPassword(const std::string& password, const std::string& salt) {
    std::string actualSalt = salt;
    if (actualSalt.empty()) {
        unsigned char saltBytes[16];
        if (RAND_bytes(saltBytes, sizeof(saltBytes)) == 1) {
            actualSalt = bytesToHex(saltBytes, sizeof(saltBytes));
        } else {
            actualSalt = "mathan_mart_secure_salt_default";
        }
    }

    unsigned char outHash[32];
    int iterations = 10000;
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.length()),
                          reinterpret_cast<const unsigned char*>(actualSalt.c_str()),
                          static_cast<int>(actualSalt.length()),
                          iterations,
                          EVP_sha256(),
                          sizeof(outHash),
                          outHash) != 1) {
        return actualSalt + "$fallback_hash";
    }

    return actualSalt + "$" + bytesToHex(outHash, sizeof(outHash));
}

bool AuthService::verifyPassword(const std::string& password, const std::string& storedHash) {
    size_t delimPos = storedHash.find('$');
    if (delimPos == std::string::npos) {
        return password == storedHash;
    }

    std::string salt = storedHash.substr(0, delimPos);
    std::string calculated = hashPassword(password, salt);
    return calculated == storedHash;
}

std::string AuthService::generateToken() {
    unsigned char tokenBytes[32];
    if (RAND_bytes(tokenBytes, sizeof(tokenBytes)) == 1) {
        return bytesToHex(tokenBytes, sizeof(tokenBytes));
    }
    static uint64_t counter = 1000;
    std::stringstream ss;
    ss << "token_" << std::chrono::system_clock::now().time_since_epoch().count()
       << "_" << ++counter;
    return ss.str();
}

AuthService::AuthResult AuthService::registerUser(const std::string& name,
                                                   const std::string& email,
                                                   const std::string& password,
                                                   const std::string& role) {
    AuthResult res;

    // Validation
    if (name.length() < 2) {
        res.success = false; res.message = "Name must be at least 2 characters long."; res.statusCode = 400;
        return res;
    }
    static const std::regex emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    if (!std::regex_match(email, emailRegex)) {
        res.success = false; res.message = "Please enter a valid email address."; res.statusCode = 400;
        return res;
    }
    if (password.length() < 6) {
        res.success = false; res.message = "Password must be at least 6 characters long."; res.statusCode = 400;
        return res;
    }

    std::string normalizedRole = role;
    for (auto& c : normalizedRole) c = static_cast<char>(tolower(c));
    if (normalizedRole != "buyer" && normalizedRole != "seller") {
        res.success = false; res.message = "Role must be 'buyer' or 'seller'."; res.statusCode = 400;
        return res;
    }

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) {
        res.success = false; res.message = "Database is unavailable."; res.statusCode = 500;
        return res;
    }

    // Check if email already exists
    const char* checkVals[] = { email.c_str() };
    PGresult* pgRes = PQexecParams(conn,
        "SELECT id FROM users WHERE email = $1",
        1, nullptr, checkVals, nullptr, nullptr, 0);
    if (PQresultStatus(pgRes) == PGRES_TUPLES_OK && PQntuples(pgRes) > 0) {
        PQclear(pgRes);
        res.success = false; res.message = "Email is already registered. Please login instead."; res.statusCode = 409;
        return res;
    }
    PQclear(pgRes);

    // Hash password and insert, return new id
    std::string passwordHash = hashPassword(password);
    const char* insVals[] = { name.c_str(), email.c_str(), passwordHash.c_str(), normalizedRole.c_str() };
    pgRes = PQexecParams(conn,
        "INSERT INTO users (name, email, password_hash, role) VALUES ($1,$2,$3,$4) RETURNING id",
        4, nullptr, insVals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_TUPLES_OK || PQntuples(pgRes) == 0) {
        std::cerr << "[AuthService] Insert error: " << PQerrorMessage(conn) << std::endl;
        PQclear(pgRes);
        res.success = false; res.message = "Failed to create user account."; res.statusCode = 500;
        return res;
    }

    int userId = std::stoi(PQgetvalue(pgRes, 0, 0));
    PQclear(pgRes);

    // Create session token
    std::string token = generateToken();
    {
        std::lock_guard<std::mutex> sLock(sessionMutex_);
        SessionInfo sess;
        sess.userId    = userId;
        sess.name      = name;
        sess.email     = email;
        sess.role      = normalizedRole;
        sess.expiresAt = std::chrono::system_clock::now() + std::chrono::hours(24);
        activeSessions_[token] = sess;
    }

    res.success    = true;
    res.message    = "Registration successful.";
    res.token      = token;
    res.statusCode = 201;
    res.user.id    = userId;
    res.user.name  = name;
    res.user.email = email;
    res.user.role  = normalizedRole;

    return res;
}

AuthService::AuthResult AuthService::loginUser(const std::string& email,
                                                const std::string& password) {
    AuthResult res;

    if (email.empty() || password.empty()) {
        res.success = false; res.message = "Email and password are required."; res.statusCode = 400;
        return res;
    }

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) {
        res.success = false; res.message = "Database unavailable."; res.statusCode = 500;
        return res;
    }

    const char* vals[] = { email.c_str() };
    PGresult* pgRes = PQexecParams(conn,
        "SELECT id, name, email, password_hash, role, created_at::text "
        "FROM users WHERE email = $1",
        1, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_TUPLES_OK || PQntuples(pgRes) == 0) {
        PQclear(pgRes);
        res.success = false; res.message = "Invalid email or password."; res.statusCode = 401;
        return res;
    }

    int         id          = std::stoi(PQgetvalue(pgRes, 0, 0));
    std::string name        = PQgetvalue(pgRes, 0, 1);
    std::string dbEmail     = PQgetvalue(pgRes, 0, 2);
    std::string storedHash  = PQgetvalue(pgRes, 0, 3);
    std::string role        = PQgetvalue(pgRes, 0, 4);
    std::string createdAt   = PQgetisnull(pgRes, 0, 5) ? "" : PQgetvalue(pgRes, 0, 5);
    PQclear(pgRes);

    if (!verifyPassword(password, storedHash)) {
        res.success = false; res.message = "Invalid email or password."; res.statusCode = 401;
        return res;
    }

    std::string token = generateToken();
    {
        std::lock_guard<std::mutex> sLock(sessionMutex_);
        SessionInfo sess;
        sess.userId    = id;
        sess.name      = name;
        sess.email     = dbEmail;
        sess.role      = role;
        sess.expiresAt = std::chrono::system_clock::now() + std::chrono::hours(24);
        activeSessions_[token] = sess;
    }

    res.success        = true;
    res.message        = "Login successful.";
    res.token          = token;
    res.statusCode     = 200;
    res.user.id        = id;
    res.user.name      = name;
    res.user.email     = dbEmail;
    res.user.role      = role;
    res.user.createdAt = createdAt;

    return res;
}

void AuthService::logout(const std::string& token) {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    activeSessions_.erase(token);
}

std::optional<SessionInfo> AuthService::validateToken(const std::string& token) {
    if (token.empty()) return std::nullopt;

    std::lock_guard<std::mutex> lock(sessionMutex_);
    auto it = activeSessions_.find(token);
    if (it == activeSessions_.end()) return std::nullopt;

    auto now = std::chrono::system_clock::now();
    if (it->second.expiresAt < now) {
        activeSessions_.erase(it);
        return std::nullopt;
    }

    return it->second;
}

std::optional<User> AuthService::getUserById(int userId) {
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return std::nullopt;

    std::string idStr   = std::to_string(userId);
    const char* vals[]  = { idStr.c_str() };
    PGresult* pgRes = PQexecParams(conn,
        "SELECT id, name, email, role, created_at::text FROM users WHERE id = $1",
        1, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_TUPLES_OK || PQntuples(pgRes) == 0) {
        PQclear(pgRes);
        return std::nullopt;
    }

    User u;
    u.id        = std::stoi(PQgetvalue(pgRes, 0, 0));
    u.name      = PQgetvalue(pgRes, 0, 1);
    u.email     = PQgetvalue(pgRes, 0, 2);
    u.role      = PQgetvalue(pgRes, 0, 3);
    u.createdAt = PQgetisnull(pgRes, 0, 4) ? "" : PQgetvalue(pgRes, 0, 4);
    PQclear(pgRes);
    return u;
}

} // namespace mathan_mart
