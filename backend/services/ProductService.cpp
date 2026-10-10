#include "ProductService.h"
#include "Database.h"
#include <iostream>
#include <sstream>

namespace mathan_mart {

ProductService& ProductService::getInstance() {
    static ProductService instance;
    return instance;
}

// ─── Helper: parse a Product row from a PGresult ─────────────────────────────
// Column order must match every SELECT in this file:
// 0:id 1:seller_id 2:seller_name 3:name 4:description
// 5:price 6:category 7:stock 8:image_url 9:created_at
// 10:avg_rating 11:review_count
static Product parseProductRow(PGresult* res, int row) {
    Product p;
    p.id            = std::stoi(PQgetvalue(res, row, 0));
    p.sellerId      = std::stoi(PQgetvalue(res, row, 1));
    p.sellerName    = PQgetvalue(res, row, 2);
    p.name          = PQgetvalue(res, row, 3);
    p.description   = PQgetisnull(res, row, 4) ? "" : PQgetvalue(res, row, 4);
    p.price         = std::stod(PQgetvalue(res, row, 5));
    p.category      = PQgetvalue(res, row, 6);
    p.stock         = std::stoi(PQgetvalue(res, row, 7));
    p.imageUrl      = PQgetisnull(res, row, 8) ? "" : PQgetvalue(res, row, 8);
    p.createdAt     = PQgetisnull(res, row, 9) ? "" : PQgetvalue(res, row, 9);
    p.averageRating = std::stod(PQgetvalue(res, row, 10));
    p.reviewCount   = std::stoi(PQgetvalue(res, row, 11));
    return p;
}

// ─── getProducts ─────────────────────────────────────────────────────────────

std::vector<Product> ProductService::getProducts(const std::string& search,
                                                  const std::string& category,
                                                  int sellerId) {
    std::vector<Product> list;
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return list;

    // Build query dynamically; collect bind values in order
    std::string sql =
        "SELECT p.id, p.seller_id, COALESCE(u.name,'Seller'), p.name, p.description, "
        "p.price, p.category, p.stock, p.image_url, p.created_at::text, "
        "COALESCE(AVG(r.rating),0.0) AS avg_rating, COUNT(r.id) AS review_count "
        "FROM products p "
        "LEFT JOIN users u ON p.seller_id = u.id "
        "LEFT JOIN reviews r ON p.id = r.product_id "
        "WHERE 1=1 ";

    std::vector<std::string> bindStorage;
    std::vector<const char*> bindPtrs;
    int paramIdx = 1;

    if (!search.empty()) {
        std::string pattern = "%" + search + "%";
        bindStorage.push_back(pattern);
        bindStorage.push_back(pattern);
        sql += " AND (p.name ILIKE $" + std::to_string(paramIdx) +
               " OR p.description ILIKE $" + std::to_string(paramIdx + 1) + ")";
        paramIdx += 2;
    }
    if (!category.empty() && category != "all" && category != "All") {
        bindStorage.push_back(category);
        sql += " AND LOWER(p.category) = LOWER($" + std::to_string(paramIdx) + ")";
        paramIdx++;
    }
    if (sellerId > 0) {
        bindStorage.push_back(std::to_string(sellerId));
        sql += " AND p.seller_id = $" + std::to_string(paramIdx);
        paramIdx++;
    }

    sql += " GROUP BY p.id, u.name ORDER BY p.id DESC";

    for (const auto& s : bindStorage) bindPtrs.push_back(s.c_str());

    PGresult* res = PQexecParams(conn, sql.c_str(),
        static_cast<int>(bindPtrs.size()),
        nullptr,
        bindPtrs.empty() ? nullptr : bindPtrs.data(),
        nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "[ProductService] getProducts error: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return list;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) list.push_back(parseProductRow(res, i));
    PQclear(res);
    return list;
}

// ─── getProductById ───────────────────────────────────────────────────────────

std::optional<Product> ProductService::getProductById(int id) {
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return std::nullopt;

    std::string idStr  = std::to_string(id);
    const char* vals[] = { idStr.c_str() };

    PGresult* res = PQexecParams(conn,
        "SELECT p.id, p.seller_id, COALESCE(u.name,'Seller'), p.name, p.description, "
        "p.price, p.category, p.stock, p.image_url, p.created_at::text, "
        "COALESCE(AVG(r.rating),0.0) AS avg_rating, COUNT(r.id) AS review_count "
        "FROM products p "
        "LEFT JOIN users u ON p.seller_id = u.id "
        "LEFT JOIN reviews r ON p.id = r.product_id "
        "WHERE p.id = $1 "
        "GROUP BY p.id, u.name",
        1, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return std::nullopt;
    }

    Product p = parseProductRow(res, 0);
    PQclear(res);
    return p;
}

// ─── createProduct ────────────────────────────────────────────────────────────

ProductService::ServiceResult ProductService::createProduct(int sellerId,
                                                             const std::string& name,
                                                             const std::string& description,
                                                             double price,
                                                             const std::string& category,
                                                             int stock,
                                                             const std::string& imageUrl) {
    ServiceResult res;

    if (name.empty()) {
        res.success = false; res.message = "Product name cannot be empty."; res.statusCode = 400; return res;
    }
    if (price <= 0.0) {
        res.success = false; res.message = "Product price must be greater than zero."; res.statusCode = 400; return res;
    }
    if (stock < 0) {
        res.success = false; res.message = "Stock cannot be negative."; res.statusCode = 400; return res;
    }
    if (category.empty()) {
        res.success = false; res.message = "Product category is required."; res.statusCode = 400; return res;
    }

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) {
        res.success = false; res.message = "Database unavailable."; res.statusCode = 500; return res;
    }

    std::string sellerIdStr = std::to_string(sellerId);
    std::string priceStr    = std::to_string(price);
    std::string stockStr    = std::to_string(stock);

    const char* vals[] = {
        sellerIdStr.c_str(), name.c_str(), description.c_str(),
        priceStr.c_str(), category.c_str(), stockStr.c_str(), imageUrl.c_str()
    };

    PGresult* pgRes = PQexecParams(conn,
        "INSERT INTO products (seller_id, name, description, price, category, stock, image_url) "
        "VALUES ($1,$2,$3,$4,$5,$6,$7) RETURNING id",
        7, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_TUPLES_OK || PQntuples(pgRes) == 0) {
        std::cerr << "[ProductService] createProduct error: " << PQerrorMessage(conn) << std::endl;
        PQclear(pgRes);
        res.success = false; res.message = "Failed to add product."; res.statusCode = 500; return res;
    }

    int newId = std::stoi(PQgetvalue(pgRes, 0, 0));
    PQclear(pgRes);

    res.success             = true;
    res.message             = "Product added successfully.";
    res.statusCode          = 201;
    res.product.id          = newId;
    res.product.sellerId    = sellerId;
    res.product.name        = name;
    res.product.description = description;
    res.product.price       = price;
    res.product.category    = category;
    res.product.stock       = stock;
    res.product.imageUrl    = imageUrl;

    return res;
}

// ─── updateProduct ────────────────────────────────────────────────────────────

ProductService::ServiceResult ProductService::updateProduct(int productId,
                                                             int currentUserId,
                                                             const std::string& currentUserRole,
                                                             const std::string& name,
                                                             const std::string& description,
                                                             double price,
                                                             const std::string& category,
                                                             int stock,
                                                             const std::string& imageUrl) {
    ServiceResult res;

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) {
        res.success = false; res.message = "Database unavailable."; res.statusCode = 500; return res;
    }

    // Verify product exists and get owner
    std::string productIdStr = std::to_string(productId);
    const char* findVals[]   = { productIdStr.c_str() };
    PGresult* pgRes = PQexecParams(conn,
        "SELECT seller_id FROM products WHERE id = $1",
        1, nullptr, findVals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_TUPLES_OK || PQntuples(pgRes) == 0) {
        PQclear(pgRes);
        res.success = false; res.message = "Product not found."; res.statusCode = 404; return res;
    }

    int ownerSellerId = std::stoi(PQgetvalue(pgRes, 0, 0));
    PQclear(pgRes);

    if (currentUserRole != "admin" && ownerSellerId != currentUserId) {
        res.success = false; res.message = "Unauthorized: You can only edit your own products."; res.statusCode = 403; return res;
    }
    if (price <= 0.0) {
        res.success = false; res.message = "Product price must be greater than zero."; res.statusCode = 400; return res;
    }
    if (stock < 0) {
        res.success = false; res.message = "Stock cannot be negative."; res.statusCode = 400; return res;
    }

    std::string priceStr = std::to_string(price);
    std::string stockStr = std::to_string(stock);
    const char* updVals[] = {
        name.c_str(), description.c_str(), priceStr.c_str(),
        category.c_str(), stockStr.c_str(), imageUrl.c_str(),
        productIdStr.c_str()
    };

    pgRes = PQexecParams(conn,
        "UPDATE products SET name=$1, description=$2, price=$3, category=$4, stock=$5, image_url=$6 "
        "WHERE id=$7",
        7, nullptr, updVals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_COMMAND_OK) {
        std::cerr << "[ProductService] updateProduct error: " << PQerrorMessage(conn) << std::endl;
        PQclear(pgRes);
        res.success = false; res.message = "Failed to update product."; res.statusCode = 500; return res;
    }
    PQclear(pgRes);

    res.success             = true;
    res.message             = "Product updated successfully.";
    res.statusCode          = 200;
    res.product.id          = productId;
    res.product.sellerId    = ownerSellerId;
    res.product.name        = name;
    res.product.description = description;
    res.product.price       = price;
    res.product.category    = category;
    res.product.stock       = stock;
    res.product.imageUrl    = imageUrl;

    return res;
}

// ─── deleteProduct ────────────────────────────────────────────────────────────

ProductService::ServiceResult ProductService::deleteProduct(int productId,
                                                             int currentUserId,
                                                             const std::string& currentUserRole) {
    ServiceResult res;

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) {
        res.success = false; res.message = "Database unavailable."; res.statusCode = 500; return res;
    }

    std::string productIdStr = std::to_string(productId);
    const char* findVals[]   = { productIdStr.c_str() };

    PGresult* pgRes = PQexecParams(conn,
        "SELECT seller_id FROM products WHERE id = $1",
        1, nullptr, findVals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_TUPLES_OK || PQntuples(pgRes) == 0) {
        PQclear(pgRes);
        res.success = false; res.message = "Product not found."; res.statusCode = 404; return res;
    }

    int ownerSellerId = std::stoi(PQgetvalue(pgRes, 0, 0));
    PQclear(pgRes);

    if (currentUserRole != "admin" && ownerSellerId != currentUserId) {
        res.success = false; res.message = "Unauthorized: You can only delete your own products."; res.statusCode = 403; return res;
    }

    pgRes = PQexecParams(conn,
        "DELETE FROM products WHERE id = $1",
        1, nullptr, findVals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_COMMAND_OK) {
        std::cerr << "[ProductService] deleteProduct error: " << PQerrorMessage(conn) << std::endl;
        PQclear(pgRes);
        res.success = false; res.message = "Failed to delete product."; res.statusCode = 500; return res;
    }
    PQclear(pgRes);

    res.success    = true;
    res.message    = "Product deleted successfully.";
    res.statusCode = 200;
    return res;
}

} // namespace mathan_mart
