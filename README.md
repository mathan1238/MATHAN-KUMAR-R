# Mathan Mart - Full-Stack E-Commerce Platform

A production-grade, individual multi-seller online shopping capstone application built with **C++20**, the **Drogon Framework**, **SQLite** (with clean abstraction for PostgreSQL), and a clean, responsive **HTML5 / CSS3 / Vanilla JavaScript** frontend.

---

## 🏛️ System Architecture

```text
+-------------------------------------------------------------+
|                     BROWSER / FRONTEND                      |
|   HTML5  *  CSS3  *  Vanilla JavaScript (ES6+)  * Fetch API  |
+-------------------------------------------------------------+
                              |
                              | HTTP REST API (JSON)
                              v
+-------------------------------------------------------------+
|                   C++20 DROGON REST SERVER                  |
|  * Static File Server (Document Root: frontend/)            |
|  * Controllers: Auth, Product, Cart, Order, Review, Admin   |
|  * Middleware: AuthMiddleware (Bearer Token & RBAC)         |
|  * Services: AuthService, ProductService, OrderService      |
|  * Security: PBKDF2-HMAC-SHA256 Password Hashing            |
+-------------------------------------------------------------+
                              |
                              | Prepared Statements & Transactions
                              v
+-------------------------------------------------------------+
|                      DATABASE LAYER                         |
|   SQLite3 (data/mathan_mart.db) with Foreign Keys & WAL     |
|   Auto-Schema Initialization & Seed Population              |
+-------------------------------------------------------------+
```

---

## 🚀 Key Features

- **Multi-Role RBAC (Role-Based Access Control)**:
  - **Buyer**: Browse catalog, live search, category filter, view product details with average star rating, manage cart, transactional checkout, order history, write verified product reviews.
  - **Seller**: Seller dashboard, add/edit/delete owned products, track received customer orders, fulfill order statuses.
  - **Admin**: System-wide dashboard metrics (total users, sellers, buyers, products, orders, sales revenue), moderate catalog (delete inappropriate products), view registered accounts and global order ledger.
- **Server-Side Security**:
  - Secure PBKDF2-HMAC-SHA256 password hashing with cryptographic salts (passwords are never stored in plaintext).
  - Cryptographically secure 64-char Bearer tokens with expiration.
  - Role enforcement on every protected endpoint (roles sent from client are never trusted).
  - Server-side price and order total calculation (client prices are ignored).
  - Server-side stock checking before adding to cart and during checkout.
  - Atomic database transactions for checkout (`BEGIN TRANSACTION; ... COMMIT; / ROLLBACK;`).
  - Parameter-bound prepared statements to prevent SQL injection.
- **Single-Server Drogon Serving**:
  - Drogon serves both REST APIs (`/api/...`) and static frontend assets (`http://localhost:8080/`) simultaneously.
  - No separate Node.js or frontend development server required.

---

## 📁 Project Directory Structure

```text
Mathan_Mart/
├── CMakeLists.txt
├── README.md
├── .gitignore
│
├── backend/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── config.json
│   │
│   ├── controllers/
│   │   ├── AuthController.h / .cpp
│   │   ├── ProductController.h / .cpp
│   │   ├── CartController.h / .cpp
│   │   ├── OrderController.h / .cpp
│   │   ├── ReviewController.h / .cpp
│   │   └── AdminController.h / .cpp
│   │
│   ├── models/
│   │   ├── User.h
│   │   ├── Product.h
│   │   ├── CartItem.h
│   │   ├── Order.h
│   │   ├── OrderItem.h
│   │   └── Review.h
│   │
│   ├── services/
│   │   ├── Database.h / .cpp
│   │   ├── AuthService.h / .cpp
│   │   ├── ProductService.h / .cpp
│   │   └── OrderService.h / .cpp
│   │
│   └── middleware/
│       ├── AuthMiddleware.h / .cpp
│
├── database/
│   ├── schema.sql
│   └── seed.sql
│
├── frontend/
│   ├── index.html
│   ├── login.html
│   ├── register.html
│   ├── products.html
│   ├── product-details.html
│   ├── cart.html
│   ├── checkout.html
│   ├── orders.html
│   ├── seller-dashboard.html
│   ├── admin-dashboard.html
│   │
│   ├── css/
│   │   ├── style.css
│   │   ├── auth.css
│   │   ├── dashboard.css
│   │   └── responsive.css
│   │
│   └── js/
│       ├── api.js
│       ├── auth.js
│       ├── products.js
│       ├── cart.js
│       ├── checkout.js
│       ├── orders.js
│       ├── seller.js
│       ├── admin.js
│       └── reviews.js
│
└── data/
    └── mathan_mart.db
```

---

## 🔑 Default Test Credentials

| Role | Email | Password | Pre-populated Data |
| :--- | :--- | :--- | :--- |
| **Admin** | `admin@mathanmart.com` | `Admin@123` | Full dashboard access, stats, moderation |
| **Seller** | `seller@mathanmart.com` | `Seller@123` | Pre-stocked with 12 flagship products |
| **Buyer** | `buyer@mathanmart.com` | `Buyer@123` | Sample account ready for checkout/reviews |

*Note: The login page includes convenient Quick-Fill buttons for instant testing.*

---

## 🛠️ Build & Run Instructions

### Prerequisites
- Windows 10/11
- MinGW-w64 GCC (with C++20 support)
- CMake 3.20+
- Ninja

### 1. Build
Open PowerShell in the project directory:
```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

### 2. Run
Make sure runtime DLLs are in your `PATH` and start the server:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
.\build\backend\MathanMartBackend.exe
```

### 3. Open in Browser
Visit:
```text
http://localhost:8080/
```

---

## 📡 REST API Reference

### Authentication
- `POST /api/auth/register` — Register a new buyer or seller (`name`, `email`, `password`, `role`)
- `POST /api/auth/login` — Sign in with email and password, returns Bearer token
- `POST /api/auth/logout` — Invalidate current session
- `GET /api/auth/me` — Retrieve authenticated user profile

### Products
- `GET /api/products` — Get catalog with optional filters (`?search=...`, `?category=...`)
- `GET /api/products/{id}` — Get detailed product info, seller, and aggregated reviews
- `POST /api/products` *(Seller only)* — Add new product
- `PUT /api/products/{id}` *(Seller / Admin)* — Update product details
- `DELETE /api/products/{id}` *(Seller / Admin)* — Delete product
- `GET /api/seller/my-products` *(Seller only)* — Get current seller's catalog

### Shopping Cart
- `GET /api/cart` *(Buyer only)* — View shopping cart with server-calculated total
- `POST /api/cart` *(Buyer only)* — Add product to cart (`productId`, `quantity`)
- `PUT /api/cart/{productId}` *(Buyer only)* — Update item quantity
- `DELETE /api/cart/{productId}` *(Buyer only)* — Remove item from cart
- `DELETE /api/cart` *(Buyer only)* — Empty cart

### Orders & Checkout
- `POST /api/orders/checkout` *(Buyer only)* — Atomic transactional checkout
- `GET /api/orders` *(Buyer only)* — View buyer's order history
- `GET /api/orders/{id}` — View single order breakdown
- `GET /api/seller/orders` *(Seller only)* — View orders received for seller items
- `GET /api/admin/orders` *(Admin only)* — View global order records
- `PUT /api/orders/{id}/status` *(Seller / Admin)* — Update order status (Pending, Processing, Shipped, Delivered, Cancelled)

### Reviews
- `GET /api/products/{productId}/reviews` — View all customer reviews for a product
- `POST /api/products/{productId}/reviews` *(Buyer only)* — Submit verified review (enforces purchase verification)

### Administration
- `GET /api/admin/stats` *(Admin only)* — Platform analytics (Users, Sellers, Buyers, Products, Orders, Sales)
- `GET /api/admin/users` *(Admin only)* — View all registered accounts
- `GET /api/admin/products` *(Admin only)* — View all catalog items
- `GET /api/admin/orders` *(Admin only)* — View all platform transactions
- `DELETE /api/admin/products/{id}` *(Admin only)* — Administrative product deletion
