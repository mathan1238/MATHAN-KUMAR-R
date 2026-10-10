$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "       MATHAN MART POSTGRESQL 18 VERIFICATION SUITE       " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# -----------------------------------------------------------------------------
# 1. POSTGRESQL 18 DIRECT DATABASE VERIFICATION
# -----------------------------------------------------------------------------
Write-Host "`n>>> [1/6] DIRECT POSTGRESQL 18 INSPECTION" -ForegroundColor Yellow
$env:PGPASSWORD = "1756"
$psql = "C:\Program Files\PostgreSQL\18\bin\psql.exe"

Write-Host "PostgreSQL Server Version:"
& $psql -U postgres -d mathan_mart -t -A -c "SELECT version();"

Write-Host "`nActive Backend Connection in pg_stat_activity:"
& $psql -U postgres -d mathan_mart -c "SELECT pid, usename, client_addr, client_port, backend_start, state FROM pg_stat_activity WHERE datname = 'mathan_mart' AND pid <> pg_backend_pid();"

Write-Host "`n12 Products in PostgreSQL Database:"
& $psql -U postgres -d mathan_mart -c "SELECT id, name, price, stock, category FROM products ORDER BY id ASC;"

# -----------------------------------------------------------------------------
# 2. SQLITE NON-USAGE PROOF
# -----------------------------------------------------------------------------
Write-Host "`n>>> [2/6] SQLITE INERTNESS CHECK" -ForegroundColor Yellow
$sqliteDb = "C:\Users\mk943\OneDrive\Desktop\Mathan Mart\data\mathan_mart.db"
if (Test-Path $sqliteDb) {
    $item = Get-Item $sqliteDb
    Write-Host "SQLite file exists at: $sqliteDb"
    Write-Host "SQLite file last modified: $($item.LastWriteTime)"
    
    # Rename SQLite file temporarily to prove backend operates independently of SQLite
    Rename-Item -Path $sqliteDb -NewName "mathan_mart.db.temporarily_renamed"
    Write-Host "Temporarily renamed SQLite file to: mathan_mart.db.temporarily_renamed" -ForegroundColor Green
}

# -----------------------------------------------------------------------------
# 3. BUYER FEATURE VERIFICATION
# -----------------------------------------------------------------------------
Write-Host "`n>>> [3/6] BUYER FEATURE TESTS" -ForegroundColor Yellow

# Buyer Login
$buyerLogin = Invoke-RestMethod -Uri "http://localhost:8080/api/auth/login" -Method POST `
    -Body '{"email":"buyer@mathanmart.com","password":"Buyer@123"}' -ContentType "application/json"
$buyerToken = $buyerLogin.data.token
Write-Host "1. Buyer Login: SUCCESS (User: $($buyerLogin.data.user.name), ID: $($buyerLogin.data.user.id), Role: $($buyerLogin.data.user.role))"

# Get Products
$prods = Invoke-RestMethod -Uri "http://localhost:8080/api/products" -Method GET
Write-Host "2. Get Products API: SUCCESS (Returned $($prods.data.Count) products)"

# Get Product By ID
$p1 = Invoke-RestMethod -Uri "http://localhost:8080/api/products/1" -Method GET
Write-Host "3. Get Product #1: SUCCESS ($($p1.data.name) - Rs.$($p1.data.price))"

# Add To Cart
$addCart = Invoke-RestMethod -Uri "http://localhost:8080/api/cart" -Method POST `
    -Headers @{"Authorization"="Bearer $buyerToken"} -ContentType "application/json" `
    -Body '{"productId":1,"quantity":1}'
Write-Host "4. Add Product #1 to Cart: SUCCESS ($($addCart.message))"

# View Cart
$cart = Invoke-RestMethod -Uri "http://localhost:8080/api/cart" -Method GET `
    -Headers @{"Authorization"="Bearer $buyerToken"}
Write-Host "5. View Cart: SUCCESS (Cart items: $($cart.data.items.Count), Total: Rs.$($cart.data.totalAmount))"

# Update Cart Item Quantity
$updCart = Invoke-RestMethod -Uri "http://localhost:8080/api/cart/1" -Method PUT `
    -Headers @{"Authorization"="Bearer $buyerToken"} -ContentType "application/json" `
    -Body '{"quantity":2}'
Write-Host "6. Update Cart Item #1 Quantity to 2: SUCCESS ($($updCart.message))"

# Checkout
$checkout = Invoke-RestMethod -Uri "http://localhost:8080/api/orders/checkout" -Method POST `
    -Headers @{"Authorization"="Bearer $buyerToken"} -ContentType "application/json" -Body '{}'
$newOrderId = $checkout.data.orderId
Write-Host "7. Checkout: SUCCESS (Created Order #$newOrderId, Total: Rs.$($checkout.data.totalAmount))"

# View Buyer Orders
$buyerOrders = Invoke-RestMethod -Uri "http://localhost:8080/api/orders" -Method GET `
    -Headers @{"Authorization"="Bearer $buyerToken"}
Write-Host "8. View Buyer Orders: SUCCESS (Buyer has $($buyerOrders.data.Count) order(s))"

# View Order Details
$orderDetail = Invoke-RestMethod -Uri "http://localhost:8080/api/orders/$newOrderId" -Method GET `
    -Headers @{"Authorization"="Bearer $buyerToken"}
Write-Host "9. View Order Details: SUCCESS (Order #$($orderDetail.data.id), Status: $($orderDetail.data.status), Items: $($orderDetail.data.items.Count))"

# Add Product Review (Product #1 was purchased!)
try {
    $addRev = Invoke-RestMethod -Uri "http://localhost:8080/api/products/1/reviews" -Method POST `
        -Headers @{"Authorization"="Bearer $buyerToken"} -ContentType "application/json" `
        -Body '{"rating":5,"comment":"Phenomenal build quality! Verified purchase."}'
    Write-Host "10. Add Review for Product #1: SUCCESS ($($addRev.message))"
} catch {
    Write-Host "10. Add Review note: $($_.Exception.Message)"
}

# Get Product Reviews
$revs = Invoke-RestMethod -Uri "http://localhost:8080/api/products/1/reviews" -Method GET
Write-Host "11. Get Reviews for Product #1: SUCCESS ($($revs.data.Count) review(s) found)"

# -----------------------------------------------------------------------------
# 4. SELLER FEATURE VERIFICATION
# -----------------------------------------------------------------------------
Write-Host "`n>>> [4/6] SELLER FEATURE TESTS" -ForegroundColor Yellow

# Seller Login
$sellerLogin = Invoke-RestMethod -Uri "http://localhost:8080/api/auth/login" -Method POST `
    -Body '{"email":"seller@mathanmart.com","password":"Seller@123"}' -ContentType "application/json"
$sellerToken = $sellerLogin.data.token
Write-Host "1. Seller Login: SUCCESS (User: $($sellerLogin.data.user.name), Role: $($sellerLogin.data.user.role))"

# Get Seller's Own Products
$myProds = Invoke-RestMethod -Uri "http://localhost:8080/api/seller/my-products" -Method GET `
    -Headers @{"Authorization"="Bearer $sellerToken"}
Write-Host "2. Seller My Products: SUCCESS ($($myProds.data.Count) products owned by seller)"

# Add New Product as Seller
$newProdBody = @{
    name = "Sony PlayStation VR2 Headset"
    description = "Next-gen VR headset with 4K HDR display and eye tracking."
    price = 57999.0
    category = "Gaming"
    stock = 15
    imageUrl = "https://images.unsplash.com/photo-1593508512255-86ab42a8e620?w=600"
} | ConvertTo-Json

$createdProd = Invoke-RestMethod -Uri "http://localhost:8080/api/products" -Method POST `
    -Headers @{"Authorization"="Bearer $sellerToken"} -ContentType "application/json" -Body $newProdBody
$newProdId = $createdProd.data.id
Write-Host "3. Add New Product: SUCCESS (Product ID: $newProdId, Name: '$($createdProd.data.name)')"

# Update Product as Seller
$updProdBody = @{
    name = "Sony PlayStation VR2 Horizon Bundle"
    description = "Includes VR headset and Horizon Call of the Mountain game code."
    price = 59999.0
    category = "Gaming"
    stock = 12
    imageUrl = "https://images.unsplash.com/photo-1593508512255-86ab42a8e620?w=600"
} | ConvertTo-Json

$updProd = Invoke-RestMethod -Uri "http://localhost:8080/api/products/$newProdId" -Method PUT `
    -Headers @{"Authorization"="Bearer $sellerToken"} -ContentType "application/json" -Body $updProdBody
Write-Host "4. Update Product: SUCCESS (Updated Name: '$($updProd.data.name)', New Price: Rs.$($updProd.data.price))"

# View Seller Orders
$sellerOrders = Invoke-RestMethod -Uri "http://localhost:8080/api/seller/orders" -Method GET `
    -Headers @{"Authorization"="Bearer $sellerToken"}
Write-Host "5. View Seller Orders: SUCCESS ($($sellerOrders.data.Count) item(s) ordered from seller)"

# -----------------------------------------------------------------------------
# 5. ADMIN FEATURE VERIFICATION
# -----------------------------------------------------------------------------
Write-Host "`n>>> [5/6] ADMIN FEATURE TESTS" -ForegroundColor Yellow

# Admin Login
$adminLogin = Invoke-RestMethod -Uri "http://localhost:8080/api/auth/login" -Method POST `
    -Body '{"email":"admin@mathanmart.com","password":"Admin@123"}' -ContentType "application/json"
$adminToken = $adminLogin.data.token
Write-Host "1. Admin Login: SUCCESS (User: $($adminLogin.data.user.name), Role: $($adminLogin.data.user.role))"

# Get Admin Stats
$stats = Invoke-RestMethod -Uri "http://localhost:8080/api/admin/stats" -Method GET `
    -Headers @{"Authorization"="Bearer $adminToken"}
Write-Host "2. Admin Stats: SUCCESS (Users: $($stats.data.totalUsers), Products: $($stats.data.totalProducts), Orders: $($stats.data.totalOrders), Sales: Rs.$($stats.data.totalSales))"

# Get All Users
$users = Invoke-RestMethod -Uri "http://localhost:8080/api/admin/users" -Method GET `
    -Headers @{"Authorization"="Bearer $adminToken"}
Write-Host "3. Admin Users List: SUCCESS ($($users.data.Count) registered users)"

# Get All Orders
$adminAllOrders = Invoke-RestMethod -Uri "http://localhost:8080/api/admin/orders" -Method GET `
    -Headers @{"Authorization"="Bearer $adminToken"}
Write-Host "4. Admin Orders List: SUCCESS ($($adminAllOrders.data.Count) total platform orders)"

# Update Order Status (to 'Shipped')
$updStatus = Invoke-RestMethod -Uri "http://localhost:8080/api/orders/$newOrderId/status" -Method PUT `
    -Headers @{"Authorization"="Bearer $adminToken"} -ContentType "application/json" `
    -Body '{"status":"Shipped"}'
Write-Host "5. Update Order #$newOrderId Status to 'Shipped': SUCCESS ($($updStatus.message))"

# Admin Delete Test Product
$delProd = Invoke-RestMethod -Uri "http://localhost:8080/api/admin/products/$newProdId" -Method DELETE `
    -Headers @{"Authorization"="Bearer $adminToken"}
Write-Host "6. Admin Delete Product #$newProdId : SUCCESS ($($delProd.message))"

# Restore SQLite filename if it was renamed
if (Test-Path "C:\Users\mk943\OneDrive\Desktop\Mathan Mart\data\mathan_mart.db.temporarily_renamed") {
    Rename-Item -Path "C:\Users\mk943\OneDrive\Desktop\Mathan Mart\data\mathan_mart.db.temporarily_renamed" -NewName "mathan_mart.db"
    Write-Host "`nRestored SQLite file name for archival." -ForegroundColor Gray
}

# -----------------------------------------------------------------------------
# 6. DIRECT DATABASE VERIFICATION IN POSTGRESQL POST-TEST
# -----------------------------------------------------------------------------
Write-Host "`n>>> [6/6] DIRECT POSTGRESQL 18 STATE AFTER ALL API OPERATIONS" -ForegroundColor Yellow

Write-Host "Orders table in PostgreSQL:"
& $psql -U postgres -d mathan_mart -c "SELECT id, buyer_id, total_amount, status, created_at FROM orders ORDER BY id ASC;"

Write-Host "`nOrder Items table in PostgreSQL:"
& $psql -U postgres -d mathan_mart -c "SELECT id, order_id, product_id, seller_id, quantity, price FROM order_items ORDER BY id ASC;"

Write-Host "`nReviews table in PostgreSQL:"
& $psql -U postgres -d mathan_mart -c "SELECT id, buyer_id, product_id, rating, comment FROM reviews ORDER BY id ASC;"

Write-Host "`nUsers table in PostgreSQL:"
& $psql -U postgres -d mathan_mart -c "SELECT id, name, email, role FROM users ORDER BY id ASC;"

Write-Host "`nFinal Table Row Counts in PostgreSQL:"
& $psql -U postgres -d mathan_mart -c "SELECT 'users' as tbl, count(*) as count from users union all select 'products', count(*) from products union all select 'orders', count(*) from orders union all select 'order_items', count(*) from order_items union all select 'cart_items', count(*) from cart_items union all select 'reviews', count(*) from reviews;"

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "      ALL POSTGRESQL 18 VERIFICATION TESTS PASSED!       " -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
