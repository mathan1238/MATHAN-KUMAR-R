#include <drogon/drogon.h>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include "services/Database.h"

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "            MATHAN MART E-COMMERCE SERVER         " << std::endl;
    std::cout << "==================================================" << std::endl;

    // ── PostgreSQL connection string ──────────────────────────────────────────
    // Supports:
    // 1. Render / Cloud: DATABASE_URL (e.g. postgres://user:pass@host:5432/dbname)
    // 2. Discrete env vars: PGHOST, PGPORT, PGDATABASE, PGUSER, PGPASSWORD / PG_PASSWORD
    // 3. Local fallback: host=localhost port=5432 dbname=mathan_mart user=postgres
    std::string connStr;
    const char* dbUrl = std::getenv("DATABASE_URL");
    if (dbUrl && *dbUrl) {
        connStr = dbUrl;
        std::cout << "[Init] Using DATABASE_URL connection string." << std::endl;
    } else {
        const char* pg_host = std::getenv("PGHOST");
        const char* pg_port = std::getenv("PGPORT");
        const char* pg_db   = std::getenv("PGDATABASE");
        const char* pg_user = std::getenv("PGUSER");
        const char* pg_pass = std::getenv("PGPASSWORD");
        if (!pg_pass || !*pg_pass) pg_pass = std::getenv("PG_PASSWORD");

        std::string host = (pg_host && *pg_host) ? pg_host : "localhost";
        std::string portStr = (pg_port && *pg_port) ? pg_port : "5432";
        std::string dbname = (pg_db && *pg_db) ? pg_db : "mathan_mart";
        std::string user = (pg_user && *pg_user) ? pg_user : "postgres";
        std::string password = (pg_pass && *pg_pass) ? pg_pass : "";

        if (password.empty()) {
            std::cerr << "[Error] Neither DATABASE_URL nor PG_PASSWORD is set." << std::endl;
            std::cerr << "        Set it before running: $env:PG_PASSWORD = \"yourpassword\"" << std::endl;
            return 1;
        }

        const char* pg_ssl  = std::getenv("PGSSLMODE");
        std::string sslmode = (pg_ssl && *pg_ssl) ? pg_ssl : "";

        connStr = "host=" + host + " port=" + portStr + " dbname=" + dbname + " user=" + user + " password=" + password;
        if (!sslmode.empty()) {
            connStr += " sslmode=" + sslmode;
        }
    }

    // ── Frontend path detection ───────────────────────────────────────────────
    std::string frontendPath = "frontend";
    if (!std::filesystem::exists(frontendPath)) {
        if      (std::filesystem::exists("../frontend"))   frontendPath = "../frontend";
        else if (std::filesystem::exists("../../frontend")) frontendPath = "../../frontend";
    }

    std::cout << "[Init] Frontend document root: " << frontendPath << std::endl;

    // ── Initialize PostgreSQL connection ──────────────────────────────────────
    if (!mathan_mart::Database::getInstance().initialize(connStr)) {
        std::cerr << "[Error] Failed to connect to PostgreSQL database! Exiting." << std::endl;
        return 1;
    }

    // ── Port configuration ────────────────────────────────────────────────────
    // Render and cloud platforms supply PORT dynamically (e.g. 10000)
    int port = 8080;
    const char* portEnv = std::getenv("PORT");
    if (portEnv && *portEnv) {
        port = std::atoi(portEnv);
    }

    // ── Configure Drogon ─────────────────────────────────────────────────────
    drogon::app()
        .setDocumentRoot(frontendPath)
        .setHomePage("index.html")
        .setClientMaxBodySize(20 * 1024 * 1024)
        .addListener("0.0.0.0", port);

    // Root handler fallback
    drogon::app().registerHandler(
        "/",
        [frontendPath](const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            std::string indexPath = frontendPath + "/index.html";
            auto resp = drogon::HttpResponse::newFileResponse(indexPath);
            callback(resp);
        },
        {drogon::Get}
    );

    // Health check endpoint for Render / monitoring
    drogon::app().registerHandler(
        "/healthz",
        [](const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            Json::Value health;
            health["status"] = "ok";
            health["service"] = "Mathan Mart";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(health);
            callback(resp);
        },
        {drogon::Get}
    );

    std::cout << "[Server] Mathan Mart Server starting on http://0.0.0.0:" << port << std::endl;
    std::cout << "[Server] Frontend available at:  http://localhost:" << port << "/" << std::endl;
    std::cout << "[Server] REST APIs available at: http://localhost:" << port << "/api/..." << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "Default Admin:  admin@mathanmart.com  / Admin@123"  << std::endl;
    std::cout << "Default Seller: seller@mathanmart.com / Seller@123" << std::endl;
    std::cout << "Default Buyer:  buyer@mathanmart.com  / Buyer@123"  << std::endl;
    std::cout << "==================================================" << std::endl;

    drogon::app().run();

    mathan_mart::Database::getInstance().close();
    return 0;
}
