#include "AdminController.h"
#include "../services/Database.h"
#include "../services/ProductService.h"
#include "../services/OrderService.h"
#include "../middleware/AuthMiddleware.h"
#include <libpq-fe.h>
#include <string>

namespace mathan_mart {

namespace {

void addCors(const drogon::HttpResponsePtr& resp) {
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Auth-Token");
}

} // anonymous namespace

void AdminController::getUsers(const drogon::HttpRequestPtr& req, 
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

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) {
        AuthMiddleware::sendJsonError(callback, 500, "Database unavailable.");
        return;
    }

    PGresult* pgRes = PQexec(conn,
        "SELECT id, name, email, role, created_at::text FROM users ORDER BY id ASC");

    if (PQresultStatus(pgRes) != PGRES_TUPLES_OK) {
        PQclear(pgRes);
        AuthMiddleware::sendJsonError(callback, 500, "Failed to fetch users.");
        return;
    }

    Json::Value usersArr(Json::arrayValue);
    int rows = PQntuples(pgRes);
    for (int i = 0; i < rows; ++i) {
        Json::Value u;
        u["id"]        = std::stoi(PQgetvalue(pgRes, i, 0));
        u["name"]      = std::string(PQgetvalue(pgRes, i, 1));
        u["email"]     = std::string(PQgetvalue(pgRes, i, 2));
        u["role"]      = std::string(PQgetvalue(pgRes, i, 3));
        u["createdAt"] = PQgetisnull(pgRes, i, 4) ? "" : std::string(PQgetvalue(pgRes, i, 4));
        usersArr.append(u);
    }
    PQclear(pgRes);

    Json::Value res;
    res["success"] = true;
    res["data"] = usersArr;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

void AdminController::getProducts(const drogon::HttpRequestPtr& req, 
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

    auto products = ProductService::getInstance().getProducts();

    Json::Value productsArr(Json::arrayValue);
    for (const auto& p : products) {
        productsArr.append(p.toJson());
    }

    Json::Value res;
    res["success"] = true;
    res["data"] = productsArr;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

void AdminController::getOrders(const drogon::HttpRequestPtr& req, 
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

void AdminController::deleteProduct(const drogon::HttpRequestPtr& req, 
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback, 
                                    int id) {
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

    auto result = ProductService::getInstance().deleteProduct(id, session.userId, "admin");

    Json::Value res;
    res["success"] = result.success;
    res["message"] = result.message;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(result.statusCode));
    addCors(resp);
    callback(resp);
}

void AdminController::getStats(const drogon::HttpRequestPtr& req, 
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

    auto stats = OrderService::getInstance().getAdminStats();

    Json::Value res;
    res["success"] = true;
    res["data"] = stats;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    addCors(resp);
    callback(resp);
}

} // namespace mathan_mart
