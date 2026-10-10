#include "Database.h"
#include "AuthService.h"
#include <iostream>
#include <cstdlib>   // getenv

namespace mathan_mart {

Database::Database() = default;

Database::~Database() {
    close();
}

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

bool Database::initialize(const std::string& connStr) {
    std::lock_guard<std::mutex> lock(dbMutex_);

    // Build connection string: prefer caller-supplied, then env vars
    if (!connStr.empty()) {
        connStr_ = connStr;
    } else if (const char* dbUrl = std::getenv("DATABASE_URL")) {
        connStr_ = dbUrl;
    } else {
        const char* pg_host = std::getenv("PGHOST");
        const char* pg_port = std::getenv("PGPORT");
        const char* pg_db   = std::getenv("PGDATABASE");
        const char* pg_user = std::getenv("PGUSER");
        const char* pg_pass = std::getenv("PGPASSWORD");
        if (!pg_pass || !*pg_pass) pg_pass = std::getenv("PG_PASSWORD");

        std::string host = (pg_host && *pg_host) ? pg_host : "localhost";
        std::string port = (pg_port && *pg_port) ? pg_port : "5432";
        std::string dbname = (pg_db && *pg_db) ? pg_db : "mathan_mart";
        std::string user = (pg_user && *pg_user) ? pg_user : "postgres";
        std::string password = (pg_pass && *pg_pass) ? pg_pass : "";

        const char* pg_ssl  = std::getenv("PGSSLMODE");
        std::string sslmode = (pg_ssl && *pg_ssl) ? pg_ssl : "";

        connStr_ = "host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + password;
        if (!sslmode.empty()) {
            connStr_ += " sslmode=" + sslmode;
        }
    }

    conn_ = PQconnectdb(connStr_.c_str());

    if (PQstatus(conn_) != CONNECTION_OK) {
        std::cerr << "[Database] Cannot connect to PostgreSQL: "
                  << PQerrorMessage(conn_) << std::endl;
        PQfinish(conn_);
        conn_ = nullptr;
        return false;
    }

    std::cout << "[Database] Connected to PostgreSQL database: mathan_mart" << std::endl;

    createSchemaIfNotExist();
    seedDataIfEmpty();

    return true;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (conn_) {
        PQfinish(conn_);
        conn_ = nullptr;
        std::cout << "[Database] PostgreSQL connection closed." << std::endl;
    }
}

PGconn* Database::getDbHandle() {
    return conn_;
}

std::mutex& Database::getMutex() {
    return dbMutex_;
}

bool Database::execute(const std::string& sql) {
    if (!conn_) return false;
    PGresult* res = PQexec(conn_, sql.c_str());
    ExecStatusType status = PQresultStatus(res);
    bool ok = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);
    if (!ok) {
        std::cerr << "[Database] SQL error: " << PQerrorMessage(conn_)
                  << " in query: " << sql << std::endl;
    }
    PQclear(res);
    return ok;
}

bool Database::beginTransaction() {
    return execute("BEGIN");
}

bool Database::commit() {
    return execute("COMMIT");
}

bool Database::rollback() {
    return execute("ROLLBACK");
}

// ─── Schema creation ─────────────────────────────────────────────────────────

void Database::createSchemaIfNotExist() {
    const char* schemaSql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id            SERIAL PRIMARY KEY,
            name          TEXT        NOT NULL,
            email         TEXT        NOT NULL UNIQUE,
            password_hash TEXT        NOT NULL,
            role          TEXT        NOT NULL CHECK (role IN ('buyer','seller','admin')),
            created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW()
        );

        CREATE TABLE IF NOT EXISTS products (
            id          SERIAL PRIMARY KEY,
            seller_id   INTEGER          NOT NULL REFERENCES users(id) ON DELETE CASCADE,
            name        TEXT             NOT NULL,
            description TEXT             NOT NULL DEFAULT '',
            price       DOUBLE PRECISION NOT NULL,
            category    TEXT             NOT NULL,
            stock       INTEGER          NOT NULL DEFAULT 0,
            image_url   TEXT             NOT NULL DEFAULT '',
            created_at  TIMESTAMPTZ      NOT NULL DEFAULT NOW()
        );

        CREATE TABLE IF NOT EXISTS cart_items (
            id         SERIAL  PRIMARY KEY,
            buyer_id   INTEGER NOT NULL REFERENCES users(id)    ON DELETE CASCADE,
            product_id INTEGER NOT NULL REFERENCES products(id) ON DELETE CASCADE,
            quantity   INTEGER NOT NULL DEFAULT 1,
            UNIQUE (buyer_id, product_id)
        );

        CREATE TABLE IF NOT EXISTS orders (
            id           SERIAL           PRIMARY KEY,
            buyer_id     INTEGER          NOT NULL REFERENCES users(id),
            total_amount DOUBLE PRECISION NOT NULL DEFAULT 0.0,
            status       TEXT             NOT NULL DEFAULT 'Pending',
            created_at   TIMESTAMPTZ      NOT NULL DEFAULT NOW()
        );

        CREATE TABLE IF NOT EXISTS order_items (
            id         SERIAL           PRIMARY KEY,
            order_id   INTEGER          NOT NULL REFERENCES orders(id)   ON DELETE CASCADE,
            product_id INTEGER          NOT NULL REFERENCES products(id),
            seller_id  INTEGER          NOT NULL REFERENCES users(id),
            quantity   INTEGER          NOT NULL,
            price      DOUBLE PRECISION NOT NULL
        );

        CREATE TABLE IF NOT EXISTS reviews (
            id         SERIAL      PRIMARY KEY,
            buyer_id   INTEGER     NOT NULL REFERENCES users(id)    ON DELETE CASCADE,
            product_id INTEGER     NOT NULL REFERENCES products(id) ON DELETE CASCADE,
            rating     INTEGER     NOT NULL CHECK (rating BETWEEN 1 AND 5),
            comment    TEXT        NOT NULL DEFAULT '',
            created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
            UNIQUE (buyer_id, product_id)
        );

        CREATE INDEX IF NOT EXISTS idx_products_seller    ON products(seller_id);
        CREATE INDEX IF NOT EXISTS idx_products_category  ON products(category);
        CREATE INDEX IF NOT EXISTS idx_cart_buyer         ON cart_items(buyer_id);
        CREATE INDEX IF NOT EXISTS idx_orders_buyer       ON orders(buyer_id);
        CREATE INDEX IF NOT EXISTS idx_order_items_order  ON order_items(order_id);
        CREATE INDEX IF NOT EXISTS idx_order_items_seller ON order_items(seller_id);
        CREATE INDEX IF NOT EXISTS idx_reviews_product    ON reviews(product_id);
        CREATE INDEX IF NOT EXISTS idx_reviews_buyer      ON reviews(buyer_id);
    )";

    PGresult* res = PQexec(conn_, schemaSql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "[Database] Schema error: " << PQerrorMessage(conn_) << std::endl;
    } else {
        std::cout << "[Database] Schema verified / created." << std::endl;
    }
    PQclear(res);
}

// ─── Seed data ────────────────────────────────────────────────────────────────

void Database::seedDataIfEmpty() {
    // Helper: run a parameterised query and return a PGresult* (caller PQclear)
    auto execParams = [&](const char* sql,
                          int nParams,
                          const char* const* values) -> PGresult* {
        return PQexecParams(conn_, sql, nParams, nullptr,
                            values, nullptr, nullptr, 0);
    };

    auto getInt = [](PGresult* r, int row, int col) -> int {
        const char* v = PQgetvalue(r, row, col);
        return v ? std::stoi(v) : 0;
    };

    // ── 1. Seed admin ──────────────────────────────────────────────────────
    PGresult* res = PQexec(conn_, "SELECT COUNT(*) FROM users WHERE role='admin'");
    int adminCount = 0;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        adminCount = getInt(res, 0, 0);
    PQclear(res);

    if (adminCount == 0) {
        std::string adminHash = AuthService::hashPassword("Admin@123");
        const char* vals[] = {
            "System Administrator",
            "admin@mathanmart.com",
            adminHash.c_str(),
            "admin"
        };
        res = execParams(
            "INSERT INTO users (name, email, password_hash, role) "
            "VALUES ($1,$2,$3,$4) ON CONFLICT (email) DO NOTHING",
            4, vals);
        PQclear(res);
        std::cout << "[Database] Seeded admin account (admin@mathanmart.com / Admin@123)." << std::endl;
    }

    // ── 2. Seed products (only if empty) ───────────────────────────────────
    res = PQexec(conn_, "SELECT COUNT(*) FROM products");
    int productCount = 0;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        productCount = getInt(res, 0, 0);
    PQclear(res);

    if (productCount > 0) return;   // products already exist

    // Ensure seller exists
    res = PQexec(conn_, "SELECT id FROM users WHERE email='seller@mathanmart.com'");
    int sellerId = 0;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        sellerId = getInt(res, 0, 0);
    PQclear(res);

    if (sellerId == 0) {
        std::string sellerHash = AuthService::hashPassword("Seller@123");
        const char* vals[] = {
            "Apex Electronics",
            "seller@mathanmart.com",
            sellerHash.c_str(),
            "seller"
        };
        res = execParams(
            "INSERT INTO users (name, email, password_hash, role) "
            "VALUES ($1,$2,$3,$4) ON CONFLICT (email) DO NOTHING RETURNING id",
            4, vals);
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            sellerId = getInt(res, 0, 0);
        PQclear(res);
        if (sellerId == 0) {
            res = PQexec(conn_, "SELECT id FROM users WHERE email='seller@mathanmart.com'");
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
                sellerId = getInt(res, 0, 0);
            PQclear(res);
        }
        std::cout << "[Database] Seeded seller account (seller@mathanmart.com / Seller@123)." << std::endl;
    }

    // Ensure buyer exists
    res = PQexec(conn_, "SELECT id FROM users WHERE email='buyer@mathanmart.com'");
    int buyerId = 0;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        buyerId = getInt(res, 0, 0);
    PQclear(res);

    if (buyerId == 0) {
        std::string buyerHash = AuthService::hashPassword("Buyer@123");
        const char* vals[] = {
            "John Doe",
            "buyer@mathanmart.com",
            buyerHash.c_str(),
            "buyer"
        };
        res = execParams(
            "INSERT INTO users (name, email, password_hash, role) "
            "VALUES ($1,$2,$3,$4) ON CONFLICT (email) DO NOTHING RETURNING id",
            4, vals);
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            buyerId = getInt(res, 0, 0);
        PQclear(res);
        if (buyerId == 0) {
            res = PQexec(conn_, "SELECT id FROM users WHERE email='buyer@mathanmart.com'");
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
                buyerId = getInt(res, 0, 0);
            PQclear(res);
        }
        std::cout << "[Database] Seeded buyer account (buyer@mathanmart.com / Buyer@123)." << std::endl;
    }

    // Seed products
    struct SeedProduct {
        const char* name;
        const char* description;
        const char* price;
        const char* category;
        const char* stock;
        const char* imageUrl;
    };

    std::vector<SeedProduct> seedProducts = {
        {"iPhone 15 Pro Max",               "Titanium design, A17 Pro chip, 48MP main camera, 6.7-inch Super Retina XDR display.",   "134999.0", "Mobiles",    "15", "https://images.unsplash.com/photo-1695048133142-1a20484d2569?w=600"},
        {"Samsung Galaxy S24 Ultra",         "AI-powered photography, Titanium frame, Snapdragon 8 Gen 3, S-Pen included.",            "129999.0", "Mobiles",    "20", "https://images.unsplash.com/photo-1610945265064-0e34e5519bbf?w=600"},
        {"MacBook Pro 16\" M3 Max",          "Extreme performance with 16-core CPU, 40-core GPU, 36GB Unified Memory, 1TB SSD.",       "349900.0", "Laptops",    "8",  "https://images.unsplash.com/photo-1517336714731-489689fd1ca8?w=600"},
        {"Dell XPS 15 OLED",                 "13th Gen Intel Core i9, 32GB RAM, RTX 4070, 3.5K OLED InfinityEdge touch screen.",      "215000.0", "Laptops",    "10", "https://images.unsplash.com/photo-1593642632823-8f785ba67e45?w=600"},
        {"Sony WH-1000XM5",                  "Industry-leading noise canceling headphones with 2 processors, 8 mics, up to 30h.",     "29990.0",  "Audio",      "30", "https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=600"},
        {"Apple AirPods Pro (2nd Gen)",       "Active Noise Cancellation, Personalized Spatial Audio, USB-C case.",                    "24900.0",  "Audio",      "40", "https://images.unsplash.com/photo-1600294037681-c80b4cb5b434?w=600"},
        {"Apple Watch Ultra 2",              "Rugged 49mm titanium case, precision dual-frequency GPS, up to 36 hours battery.",       "89900.0",  "Smart Wear", "12", "https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600"},
        {"Samsung Galaxy Watch 6 Classic",   "Rotating bezel, advanced sleep coaching, body composition analysis, sapphire crystal.", "36999.0",  "Smart Wear", "18", "https://images.unsplash.com/photo-1579586337278-3befd40fd17a?w=600"},
        {"PlayStation 5 Slim",               "Ultra-high speed SSD, ray tracing, 4K-TV gaming, HDR, DualSense controller.",           "54990.0",  "Gaming",     "14", "https://images.unsplash.com/photo-1606813907291-d86efa9b94db?w=600"},
        {"Logitech G Pro X Superlight 2",    "Hero 2 sensor, 32,000 DPI, 95 hours battery life, lightweight 60g design.",            "14995.0",  "Gaming",     "25", "https://images.unsplash.com/photo-1615663245857-ac93bb7c39e7?w=600"},
        {"Anker 737 Power Bank (24,000mAh)", "Ultra-powerful two-way charging with 140W fast output and smart digital display.",     "12999.0",  "Accessories","35", "https://images.unsplash.com/photo-1609592807664-88481358fa26?w=600"},
        {"Keychron Q1 Pro Mechanical Keyboard","Fully customizable 75% layout, CNC aluminum body, hot-swappable switches.",          "18999.0",  "Accessories","16", "https://images.unsplash.com/photo-1587829741301-dc798b83add3?w=600"}
    };

    std::string sellerIdStr = std::to_string(sellerId);
    for (const auto& p : seedProducts) {
        const char* vals[] = {
            sellerIdStr.c_str(),
            p.name, p.description, p.price, p.category, p.stock, p.imageUrl
        };
        PGresult* r = execParams(
            "INSERT INTO products (seller_id, name, description, price, category, stock, image_url) "
            "VALUES ($1,$2,$3,$4,$5,$6,$7) ON CONFLICT DO NOTHING",
            7, vals);
        PQclear(r);
    }
    std::cout << "[Database] Seeded 12 sample products." << std::endl;

    // Seed initial review
    if (buyerId > 0) {
        res = PQexec(conn_, "SELECT id FROM products ORDER BY id ASC LIMIT 1");
        int firstProductId = 0;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
            firstProductId = getInt(res, 0, 0);
        PQclear(res);

        if (firstProductId > 0) {
            std::string buyerIdStr     = std::to_string(buyerId);
            std::string productIdStr   = std::to_string(firstProductId);
            const char* revVals[] = {
                buyerIdStr.c_str(), productIdStr.c_str(), "5",
                "Exceptional build quality and the titanium finish feels incredible. Battery lasts all day!"
            };
            res = execParams(
                "INSERT INTO reviews (buyer_id, product_id, rating, comment) "
                "VALUES ($1,$2,$3,$4) ON CONFLICT DO NOTHING",
                4, revVals);
            PQclear(res);
        }
    }
}

} // namespace mathan_mart
