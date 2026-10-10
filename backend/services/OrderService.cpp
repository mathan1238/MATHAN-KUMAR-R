#include "OrderService.h"
#include "Database.h"
#include <iostream>
#include <sstream>

namespace mathan_mart {

OrderService& OrderService::getInstance() {
    static OrderService instance;
    return instance;
}

// ─── getCart ─────────────────────────────────────────────────────────────────

std::vector<CartItem> OrderService::getCart(int buyerId, double& outTotal) {
    std::vector<CartItem> items;
    outTotal = 0.0;

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return items;

    std::string idStr  = std::to_string(buyerId);
    const char* vals[] = { idStr.c_str() };

    PGresult* res = PQexecParams(conn,
        "SELECT c.id, c.buyer_id, c.product_id, c.quantity, "
        "p.name, p.price, p.category, p.image_url, p.stock "
        "FROM cart_items c "
        "JOIN products p ON c.product_id = p.id "
        "WHERE c.buyer_id = $1 ORDER BY c.id ASC",
        1, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return items;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        CartItem item;
        item.id               = std::stoi(PQgetvalue(res, i, 0));
        item.buyerId          = std::stoi(PQgetvalue(res, i, 1));
        item.productId        = std::stoi(PQgetvalue(res, i, 2));
        item.quantity         = std::stoi(PQgetvalue(res, i, 3));
        item.productName      = PQgetvalue(res, i, 4);
        item.productPrice     = std::stod(PQgetvalue(res, i, 5));
        item.productCategory  = PQgetvalue(res, i, 6);
        item.productImageUrl  = PQgetisnull(res, i, 7) ? "" : PQgetvalue(res, i, 7);
        item.productStock     = std::stoi(PQgetvalue(res, i, 8));
        item.subtotal         = item.quantity * item.productPrice;
        outTotal             += item.subtotal;
        items.push_back(item);
    }
    PQclear(res);
    return items;
}

// ─── addToCart ────────────────────────────────────────────────────────────────

bool OrderService::addToCart(int buyerId, int productId, int quantity,
                              std::string& errorMsg, int& statusCode) {
    if (quantity <= 0) {
        errorMsg = "Quantity must be greater than zero."; statusCode = 400; return false;
    }

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) { errorMsg = "Database unavailable."; statusCode = 500; return false; }

    // 1. Check stock
    std::string pidStr = std::to_string(productId);
    const char* pVals[] = { pidStr.c_str() };
    PGresult* res = PQexecParams(conn,
        "SELECT stock FROM products WHERE id = $1",
        1, nullptr, pVals, nullptr, nullptr, 0);

    int currentStock = -1;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        currentStock = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    if (currentStock == -1) { errorMsg = "Product not found."; statusCode = 404; return false; }
    if (currentStock <= 0)  { errorMsg = "Product is out of stock."; statusCode = 400; return false; }

    // 2. Check existing cart quantity
    std::string bidStr = std::to_string(buyerId);
    const char* cVals[] = { bidStr.c_str(), pidStr.c_str() };
    res = PQexecParams(conn,
        "SELECT quantity FROM cart_items WHERE buyer_id = $1 AND product_id = $2",
        2, nullptr, cVals, nullptr, nullptr, 0);

    int existingQty = 0;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        existingQty = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    int totalDesired = existingQty + quantity;
    if (totalDesired > currentStock) {
        errorMsg = "Requested total quantity (" + std::to_string(totalDesired) +
                   ") exceeds available stock (" + std::to_string(currentStock) + ").";
        statusCode = 400;
        return false;
    }

    // 3. Upsert
    std::string totalStr = std::to_string(totalDesired);
    std::string qtyStr   = std::to_string(quantity);

    if (existingQty > 0) {
        const char* uVals[] = { totalStr.c_str(), bidStr.c_str(), pidStr.c_str() };
        res = PQexecParams(conn,
            "UPDATE cart_items SET quantity = $1 WHERE buyer_id = $2 AND product_id = $3",
            3, nullptr, uVals, nullptr, nullptr, 0);
    } else {
        const char* iVals[] = { bidStr.c_str(), pidStr.c_str(), qtyStr.c_str() };
        res = PQexecParams(conn,
            "INSERT INTO cart_items (buyer_id, product_id, quantity) VALUES ($1,$2,$3) "
            "ON CONFLICT (buyer_id, product_id) DO UPDATE SET quantity = EXCLUDED.quantity",
            3, nullptr, iVals, nullptr, nullptr, 0);
    }
    PQclear(res);
    return true;
}

// ─── updateCartItem ───────────────────────────────────────────────────────────

bool OrderService::updateCartItem(int buyerId, int productId, int quantity,
                                   std::string& errorMsg, int& statusCode) {
    if (quantity <= 0) return removeCartItem(buyerId, productId);

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) { errorMsg = "Database unavailable."; statusCode = 500; return false; }

    std::string pidStr = std::to_string(productId);
    const char* pVals[] = { pidStr.c_str() };
    PGresult* res = PQexecParams(conn,
        "SELECT stock FROM products WHERE id = $1",
        1, nullptr, pVals, nullptr, nullptr, 0);

    int stock = -1;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        stock = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    if (stock == -1) { errorMsg = "Product not found."; statusCode = 404; return false; }
    if (quantity > stock) {
        errorMsg = "Requested quantity exceeds available stock (" + std::to_string(stock) + ").";
        statusCode = 400; return false;
    }

    std::string bidStr = std::to_string(buyerId);
    std::string qtyStr = std::to_string(quantity);
    const char* uVals[] = { qtyStr.c_str(), bidStr.c_str(), pidStr.c_str() };
    res = PQexecParams(conn,
        "UPDATE cart_items SET quantity = $1 WHERE buyer_id = $2 AND product_id = $3",
        3, nullptr, uVals, nullptr, nullptr, 0);
    bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return ok;
}

// ─── removeCartItem ───────────────────────────────────────────────────────────

bool OrderService::removeCartItem(int buyerId, int productId) {
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return false;

    std::string bidStr = std::to_string(buyerId);
    std::string pidStr = std::to_string(productId);
    const char* vals[] = { bidStr.c_str(), pidStr.c_str() };
    PGresult* res = PQexecParams(conn,
        "DELETE FROM cart_items WHERE buyer_id = $1 AND product_id = $2",
        2, nullptr, vals, nullptr, nullptr, 0);
    bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return ok;
}

// ─── clearCart ────────────────────────────────────────────────────────────────

bool OrderService::clearCart(int buyerId) {
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return false;

    std::string bidStr = std::to_string(buyerId);
    const char* vals[] = { bidStr.c_str() };
    PGresult* res = PQexecParams(conn,
        "DELETE FROM cart_items WHERE buyer_id = $1",
        1, nullptr, vals, nullptr, nullptr, 0);
    bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return ok;
}

// ─── checkout ─────────────────────────────────────────────────────────────────

OrderService::CheckoutResult OrderService::checkout(int buyerId) {
    CheckoutResult res;

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) {
        res.success = false; res.message = "Database unavailable."; res.statusCode = 500; return res;
    }

    // Begin transaction
    PQexec(conn, "BEGIN");

    // 1. Fetch cart items
    struct PendingItem {
        int productId, sellerId, requestedQty, availableStock;
        double price;
        std::string productName, imageUrl;
    };

    std::string bidStr = std::to_string(buyerId);
    const char* bidVals[] = { bidStr.c_str() };

    PGresult* pgRes = PQexecParams(conn,
        "SELECT c.product_id, p.seller_id, p.name, c.quantity, p.stock, p.price, p.image_url "
        "FROM cart_items c "
        "JOIN products p ON c.product_id = p.id "
        "WHERE c.buyer_id = $1",
        1, nullptr, bidVals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_TUPLES_OK) {
        PQexec(conn, "ROLLBACK");
        PQclear(pgRes);
        res.success = false; res.message = "Failed to load cart for checkout."; res.statusCode = 500; return res;
    }

    std::vector<PendingItem> pendingItems;
    int rows = PQntuples(pgRes);
    for (int i = 0; i < rows; ++i) {
        PendingItem pi;
        pi.productId      = std::stoi(PQgetvalue(pgRes, i, 0));
        pi.sellerId       = std::stoi(PQgetvalue(pgRes, i, 1));
        pi.productName    = PQgetvalue(pgRes, i, 2);
        pi.requestedQty   = std::stoi(PQgetvalue(pgRes, i, 3));
        pi.availableStock = std::stoi(PQgetvalue(pgRes, i, 4));
        pi.price          = std::stod(PQgetvalue(pgRes, i, 5));
        pi.imageUrl       = PQgetisnull(pgRes, i, 6) ? "" : PQgetvalue(pgRes, i, 6);
        pendingItems.push_back(pi);
    }
    PQclear(pgRes);

    if (pendingItems.empty()) {
        PQexec(conn, "ROLLBACK");
        res.success = false; res.message = "Your cart is empty. Please add items before checking out."; res.statusCode = 400; return res;
    }

    // 2. Validate stock and compute total
    double totalAmount = 0.0;
    for (const auto& item : pendingItems) {
        if (item.requestedQty > item.availableStock) {
            PQexec(conn, "ROLLBACK");
            res.success = false;
            res.message = "Insufficient stock for '" + item.productName + "'. Available: " +
                          std::to_string(item.availableStock) + ", Requested: " + std::to_string(item.requestedQty);
            res.statusCode = 400;
            return res;
        }
        totalAmount += (item.requestedQty * item.price);
    }

    // 3. Create order — use RETURNING id
    std::string totalStr   = std::to_string(totalAmount);
    const char* ordVals[]  = { bidStr.c_str(), totalStr.c_str() };
    pgRes = PQexecParams(conn,
        "INSERT INTO orders (buyer_id, total_amount, status) VALUES ($1,$2,'Pending') RETURNING id",
        2, nullptr, ordVals, nullptr, nullptr, 0);

    if (PQresultStatus(pgRes) != PGRES_TUPLES_OK || PQntuples(pgRes) == 0) {
        PQexec(conn, "ROLLBACK");
        PQclear(pgRes);
        res.success = false; res.message = "Order creation failed."; res.statusCode = 500; return res;
    }
    int orderId = std::stoi(PQgetvalue(pgRes, 0, 0));
    PQclear(pgRes);

    // 4. Insert order_items and decrement stock
    std::string orderIdStr = std::to_string(orderId);
    for (const auto& pi : pendingItems) {
        std::string piProductId = std::to_string(pi.productId);
        std::string piSellerId  = std::to_string(pi.sellerId);
        std::string piQty       = std::to_string(pi.requestedQty);
        std::string piPrice     = std::to_string(pi.price);

        const char* iVals[] = {
            orderIdStr.c_str(), piProductId.c_str(),
            piSellerId.c_str(), piQty.c_str(), piPrice.c_str()
        };
        pgRes = PQexecParams(conn,
            "INSERT INTO order_items (order_id, product_id, seller_id, quantity, price) "
            "VALUES ($1,$2,$3,$4,$5)",
            5, nullptr, iVals, nullptr, nullptr, 0);
        PQclear(pgRes);

        const char* sVals[] = { piQty.c_str(), piProductId.c_str() };
        pgRes = PQexecParams(conn,
            "UPDATE products SET stock = stock - $1 WHERE id = $2",
            2, nullptr, sVals, nullptr, nullptr, 0);
        PQclear(pgRes);

        OrderItem oi;
        oi.orderId        = orderId;
        oi.productId      = pi.productId;
        oi.sellerId       = pi.sellerId;
        oi.productName    = pi.productName;
        oi.productImageUrl= pi.imageUrl;
        oi.quantity       = pi.requestedQty;
        oi.price          = pi.price;
        res.items.push_back(oi);
    }

    // 5. Clear cart
    pgRes = PQexecParams(conn,
        "DELETE FROM cart_items WHERE buyer_id = $1",
        1, nullptr, bidVals, nullptr, nullptr, 0);
    PQclear(pgRes);

    PQexec(conn, "COMMIT");

    res.success     = true;
    res.message     = "Order placed successfully!";
    res.statusCode  = 201;
    res.orderId     = orderId;
    res.totalAmount = totalAmount;
    return res;
}

// ─── Helper: parse order items for an order id ───────────────────────────────

static std::vector<OrderItem> fetchOrderItems(PGconn* conn, int orderId) {
    std::vector<OrderItem> items;
    std::string oidStr = std::to_string(orderId);
    const char* vals[] = { oidStr.c_str() };

    PGresult* res = PQexecParams(conn,
        "SELECT oi.id, oi.order_id, oi.product_id, oi.seller_id, oi.quantity, oi.price, "
        "p.name, p.image_url, u.name "
        "FROM order_items oi "
        "JOIN products p ON oi.product_id = p.id "
        "JOIN users u ON oi.seller_id = u.id "
        "WHERE oi.order_id = $1",
        1, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) { PQclear(res); return items; }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        OrderItem oi;
        oi.id             = std::stoi(PQgetvalue(res, i, 0));
        oi.orderId        = std::stoi(PQgetvalue(res, i, 1));
        oi.productId      = std::stoi(PQgetvalue(res, i, 2));
        oi.sellerId       = std::stoi(PQgetvalue(res, i, 3));
        oi.quantity       = std::stoi(PQgetvalue(res, i, 4));
        oi.price          = std::stod(PQgetvalue(res, i, 5));
        oi.productName    = PQgetvalue(res, i, 6);
        oi.productImageUrl= PQgetisnull(res, i, 7) ? "" : PQgetvalue(res, i, 7);
        oi.sellerName     = PQgetvalue(res, i, 8);
        items.push_back(oi);
    }
    PQclear(res);
    return items;
}

// ─── getBuyerOrders ───────────────────────────────────────────────────────────

std::vector<Order> OrderService::getBuyerOrders(int buyerId) {
    std::vector<Order> orders;
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return orders;

    std::string bidStr = std::to_string(buyerId);
    const char* vals[] = { bidStr.c_str() };

    PGresult* res = PQexecParams(conn,
        "SELECT o.id, o.buyer_id, u.name, u.email, o.total_amount, o.status, o.created_at::text "
        "FROM orders o JOIN users u ON o.buyer_id = u.id "
        "WHERE o.buyer_id = $1 ORDER BY o.id DESC",
        1, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) { PQclear(res); return orders; }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        Order o;
        o.id          = std::stoi(PQgetvalue(res, i, 0));
        o.buyerId     = std::stoi(PQgetvalue(res, i, 1));
        o.buyerName   = PQgetvalue(res, i, 2);
        o.buyerEmail  = PQgetvalue(res, i, 3);
        o.totalAmount = std::stod(PQgetvalue(res, i, 4));
        o.status      = PQgetvalue(res, i, 5);
        o.createdAt   = PQgetisnull(res, i, 6) ? "" : PQgetvalue(res, i, 6);
        orders.push_back(o);
    }
    PQclear(res);

    for (auto& o : orders)
        o.items = fetchOrderItems(conn, o.id);

    return orders;
}

// ─── getOrderById ─────────────────────────────────────────────────────────────

std::optional<Order> OrderService::getOrderById(int orderId, int userId,
                                                  const std::string& userRole) {
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return std::nullopt;

    std::string oidStr = std::to_string(orderId);
    const char* vals[] = { oidStr.c_str() };

    PGresult* res = PQexecParams(conn,
        "SELECT o.id, o.buyer_id, u.name, u.email, o.total_amount, o.status, o.created_at::text "
        "FROM orders o JOIN users u ON o.buyer_id = u.id WHERE o.id = $1",
        1, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res); return std::nullopt;
    }

    Order o;
    o.id          = std::stoi(PQgetvalue(res, 0, 0));
    o.buyerId     = std::stoi(PQgetvalue(res, 0, 1));
    o.buyerName   = PQgetvalue(res, 0, 2);
    o.buyerEmail  = PQgetvalue(res, 0, 3);
    o.totalAmount = std::stod(PQgetvalue(res, 0, 4));
    o.status      = PQgetvalue(res, 0, 5);
    o.createdAt   = PQgetisnull(res, 0, 6) ? "" : PQgetvalue(res, 0, 6);
    PQclear(res);

    o.items = fetchOrderItems(conn, orderId);

    bool sellerHasItem = false;
    for (const auto& oi : o.items)
        if (oi.sellerId == userId) { sellerHasItem = true; break; }

    if (userRole != "admin" && o.buyerId != userId && !sellerHasItem)
        return std::nullopt;

    return o;
}

// ─── getSellerOrderItems ──────────────────────────────────────────────────────

std::vector<OrderItem> OrderService::getSellerOrderItems(int sellerId) {
    std::vector<OrderItem> items;
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return items;

    std::string sidStr = std::to_string(sellerId);
    const char* vals[] = { sidStr.c_str() };

    PGresult* res = PQexecParams(conn,
        "SELECT oi.id, oi.order_id, oi.product_id, oi.seller_id, oi.quantity, oi.price, "
        "p.name, p.image_url, buyer.name, o.status, o.created_at::text "
        "FROM order_items oi "
        "JOIN orders o ON oi.order_id = o.id "
        "JOIN users buyer ON o.buyer_id = buyer.id "
        "JOIN products p ON oi.product_id = p.id "
        "WHERE oi.seller_id = $1 ORDER BY oi.id DESC",
        1, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) { PQclear(res); return items; }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        OrderItem oi;
        oi.id             = std::stoi(PQgetvalue(res, i, 0));
        oi.orderId        = std::stoi(PQgetvalue(res, i, 1));
        oi.productId      = std::stoi(PQgetvalue(res, i, 2));
        oi.sellerId       = std::stoi(PQgetvalue(res, i, 3));
        oi.quantity       = std::stoi(PQgetvalue(res, i, 4));
        oi.price          = std::stod(PQgetvalue(res, i, 5));
        oi.productName    = PQgetvalue(res, i, 6);
        oi.productImageUrl= PQgetisnull(res, i, 7) ? "" : PQgetvalue(res, i, 7);
        oi.sellerName     = PQgetvalue(res, i, 8); // buyer name in this context
        items.push_back(oi);
    }
    PQclear(res);
    return items;
}

// ─── getAllOrders ─────────────────────────────────────────────────────────────

std::vector<Order> OrderService::getAllOrders() {
    std::vector<Order> orders;
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return orders;

    PGresult* res = PQexec(conn,
        "SELECT o.id, o.buyer_id, u.name, u.email, o.total_amount, o.status, o.created_at::text "
        "FROM orders o JOIN users u ON o.buyer_id = u.id ORDER BY o.id DESC");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) { PQclear(res); return orders; }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        Order o;
        o.id          = std::stoi(PQgetvalue(res, i, 0));
        o.buyerId     = std::stoi(PQgetvalue(res, i, 1));
        o.buyerName   = PQgetvalue(res, i, 2);
        o.buyerEmail  = PQgetvalue(res, i, 3);
        o.totalAmount = std::stod(PQgetvalue(res, i, 4));
        o.status      = PQgetvalue(res, i, 5);
        o.createdAt   = PQgetisnull(res, i, 6) ? "" : PQgetvalue(res, i, 6);
        orders.push_back(o);
    }
    PQclear(res);

    for (auto& o : orders)
        o.items = fetchOrderItems(conn, o.id);

    return orders;
}

// ─── updateOrderStatus ────────────────────────────────────────────────────────

bool OrderService::updateOrderStatus(int orderId, const std::string& status) {
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return false;

    std::string oidStr = std::to_string(orderId);
    const char* vals[] = { status.c_str(), oidStr.c_str() };
    PGresult* res = PQexecParams(conn,
        "UPDATE orders SET status = $1 WHERE id = $2",
        2, nullptr, vals, nullptr, nullptr, 0);
    bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return ok;
}

// ─── addReview ────────────────────────────────────────────────────────────────

bool OrderService::addReview(int buyerId, int productId, int rating,
                              const std::string& comment,
                              std::string& errorMsg, int& statusCode) {
    if (rating < 1 || rating > 5) {
        errorMsg = "Rating must be between 1 and 5."; statusCode = 400; return false;
    }

    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) { errorMsg = "Database unavailable."; statusCode = 500; return false; }

    // 1. Verify purchase
    std::string bidStr = std::to_string(buyerId);
    std::string pidStr = std::to_string(productId);
    const char* pVals[] = { bidStr.c_str(), pidStr.c_str() };

    PGresult* res = PQexecParams(conn,
        "SELECT COUNT(*) FROM orders o "
        "JOIN order_items oi ON o.id = oi.order_id "
        "WHERE o.buyer_id = $1 AND oi.product_id = $2",
        2, nullptr, pVals, nullptr, nullptr, 0);

    int purchaseCount = 0;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        purchaseCount = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    if (purchaseCount == 0) {
        errorMsg = "You can only review products you have purchased."; statusCode = 403; return false;
    }

    // 2. Check duplicate
    res = PQexecParams(conn,
        "SELECT id FROM reviews WHERE buyer_id = $1 AND product_id = $2",
        2, nullptr, pVals, nullptr, nullptr, 0);

    bool alreadyReviewed = (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0);
    PQclear(res);

    if (alreadyReviewed) {
        errorMsg = "You have already submitted a review for this product."; statusCode = 409; return false;
    }

    // 3. Insert
    std::string ratingStr = std::to_string(rating);
    const char* iVals[] = { bidStr.c_str(), pidStr.c_str(), ratingStr.c_str(), comment.c_str() };
    res = PQexecParams(conn,
        "INSERT INTO reviews (buyer_id, product_id, rating, comment) VALUES ($1,$2,$3,$4)",
        4, nullptr, iVals, nullptr, nullptr, 0);

    bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!ok) {
        errorMsg = "Failed to submit review."; statusCode = 500;
    }
    PQclear(res);
    return ok;
}

// ─── getProductReviews ────────────────────────────────────────────────────────

std::vector<Review> OrderService::getProductReviews(int productId) {
    std::vector<Review> reviews;
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return reviews;

    std::string pidStr = std::to_string(productId);
    const char* vals[] = { pidStr.c_str() };

    PGresult* res = PQexecParams(conn,
        "SELECT r.id, r.buyer_id, u.name, r.product_id, r.rating, r.comment, r.created_at::text "
        "FROM reviews r JOIN users u ON r.buyer_id = u.id "
        "WHERE r.product_id = $1 ORDER BY r.id DESC",
        1, nullptr, vals, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) { PQclear(res); return reviews; }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        Review r;
        r.id        = std::stoi(PQgetvalue(res, i, 0));
        r.buyerId   = std::stoi(PQgetvalue(res, i, 1));
        r.buyerName = PQgetvalue(res, i, 2);
        r.productId = std::stoi(PQgetvalue(res, i, 3));
        r.rating    = std::stoi(PQgetvalue(res, i, 4));
        r.comment   = PQgetisnull(res, i, 5) ? "" : PQgetvalue(res, i, 5);
        r.createdAt = PQgetisnull(res, i, 6) ? "" : PQgetvalue(res, i, 6);
        reviews.push_back(r);
    }
    PQclear(res);
    return reviews;
}

// ─── getAdminStats ────────────────────────────────────────────────────────────

Json::Value OrderService::getAdminStats() {
    Json::Value stats;
    Database& db = Database::getInstance();
    std::lock_guard<std::mutex> lock(db.getMutex());
    PGconn* conn = db.getDbHandle();
    if (!conn) return stats;

    auto execCount = [&](const char* sql) -> int {
        PGresult* r = PQexec(conn, sql);
        int count = 0;
        if (PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0)
            count = std::stoi(PQgetvalue(r, 0, 0));
        PQclear(r);
        return count;
    };

    stats["totalUsers"]    = execCount("SELECT COUNT(*) FROM users");
    stats["totalSellers"]  = execCount("SELECT COUNT(*) FROM users WHERE role='seller'");
    stats["totalBuyers"]   = execCount("SELECT COUNT(*) FROM users WHERE role='buyer'");
    stats["totalProducts"] = execCount("SELECT COUNT(*) FROM products");
    stats["totalOrders"]   = execCount("SELECT COUNT(*) FROM orders");

    PGresult* r = PQexec(conn, "SELECT COALESCE(SUM(total_amount),0.0) FROM orders");
    double totalSales = 0.0;
    if (PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0)
        totalSales = std::stod(PQgetvalue(r, 0, 0));
    PQclear(r);
    stats["totalSales"] = totalSales;

    return stats;
}

} // namespace mathan_mart
