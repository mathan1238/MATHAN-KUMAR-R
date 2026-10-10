/**
 * Mathan Mart Seller Dashboard Handler
 * Manages seller products, modals for Add/Edit, product deletion, and received orders view.
 */

const seller = {
  formatINR(amount) {
    return new Intl.NumberFormat('en-IN', {
      style: 'currency',
      currency: 'INR',
      maximumFractionDigits: 0
    }).format(amount);
  },

  async init() {
    const user = auth.getCurrentUser();
    if (!user || user.role !== "seller") {
      showToast("Access restricted: Seller account required.", "error");
      setTimeout(() => {
        window.location.href = "login.html";
      }, 1000);
      return;
    }

    const sellerNameEl = document.getElementById("seller-name");
    if (sellerNameEl) sellerNameEl.textContent = user.name;

    await this.loadMyProducts();
    await this.loadSellerOrders();
  },

  async loadMyProducts() {
    const tbody = document.getElementById("seller-products-body");
    if (!tbody) return;

    const res = await api.get("/seller/my-products");

    if (!res.success || !res.data || res.data.length === 0) {
      tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; padding: 30px; color: var(--text-muted);">No products listed yet. Click "Add New Product" to list your first item.</td></tr>`;
      document.getElementById("stat-products").textContent = "0";
      return;
    }

    document.getElementById("stat-products").textContent = res.data.length;

    tbody.innerHTML = res.data.map(p => `
      <tr>
        <td>#${p.id}</td>
        <td>
          <div style="display: flex; align-items: center; gap: 10px;">
            <img src="${p.imageUrl || 'https://images.unsplash.com/photo-1526170375885-4d8ecf77b99f?w=100'}" 
                 alt="${p.name}" style="width: 40px; height: 40px; object-fit: cover; border-radius: 6px;">
            <strong style="color: var(--text-main);">${p.name}</strong>
          </div>
        </td>
        <td>${p.category}</td>
        <td><strong>${this.formatINR(p.price)}</strong></td>
        <td><span class="stock-tag ${p.stock > 0 ? 'in-stock' : 'out-of-stock'}">${p.stock} units</span></td>
        <td>⭐ ${Number(p.averageRating).toFixed(1)} (${p.reviewCount})</td>
        <td>
          <div style="display: flex; gap: 8px;">
            <button onclick="seller.openEditModal(${p.id})" class="btn btn-secondary btn-sm">Edit</button>
            <button onclick="seller.deleteProduct(${p.id})" class="btn btn-sm" style="background: #fee2e2; color: #b91c1c; border: none;">Delete</button>
          </div>
        </td>
      </tr>
    `).join("");
  },

  async loadSellerOrders() {
    const tbody = document.getElementById("seller-orders-body");
    if (!tbody) return;

    const res = await api.get("/seller/orders");

    if (!res.success || !res.data || res.data.length === 0) {
      tbody.innerHTML = `<tr><td colspan="6" style="text-align: center; padding: 30px; color: var(--text-muted);">No orders received yet.</td></tr>`;
      document.getElementById("stat-orders").textContent = "0";
      document.getElementById("stat-revenue").textContent = "₹0";
      return;
    }

    let totalRev = 0;
    res.data.forEach(item => totalRev += (item.price * item.quantity));

    document.getElementById("stat-orders").textContent = res.data.length;
    document.getElementById("stat-revenue").textContent = this.formatINR(totalRev);

    tbody.innerHTML = res.data.map(item => `
      <tr>
        <td>#MM-${item.orderId}</td>
        <td><strong>${item.productName}</strong></td>
        <td>${item.sellerName || 'Customer'}</td>
        <td>${item.quantity}</td>
        <td><strong>${this.formatINR(item.price * item.quantity)}</strong></td>
        <td>
          <select onchange="seller.updateOrderStatus(${item.orderId}, this.value)" 
                  style="padding: 4px 8px; border-radius: 6px; border: 1px solid var(--border-color); font-size: 13px;">
            <option value="Pending">Pending</option>
            <option value="Processing">Processing</option>
            <option value="Shipped">Shipped</option>
            <option value="Delivered">Delivered</option>
            <option value="Cancelled">Cancelled</option>
          </select>
        </td>
      </tr>
    `).join("");
  },

  openAddModal() {
    document.getElementById("modal-title").textContent = "Add New Product";
    document.getElementById("prod-id").value = "";
    document.getElementById("prod-name").value = "";
    document.getElementById("prod-desc").value = "";
    document.getElementById("prod-price").value = "";
    document.getElementById("prod-cat").value = "Mobiles";
    document.getElementById("prod-stock").value = "10";
    document.getElementById("prod-img").value = "";
    document.getElementById("product-modal").classList.add("active");
  },

  async openEditModal(productId) {
    const res = await api.get(`/products/${productId}`);
    if (!res.success || !res.data) {
      showToast("Failed to fetch product details.", "error");
      return;
    }

    const p = res.data;
    document.getElementById("modal-title").textContent = "Edit Product";
    document.getElementById("prod-id").value = p.id;
    document.getElementById("prod-name").value = p.name;
    document.getElementById("prod-desc").value = p.description || "";
    document.getElementById("prod-price").value = p.price;
    document.getElementById("prod-cat").value = p.category;
    document.getElementById("prod-stock").value = p.stock;
    document.getElementById("prod-img").value = p.imageUrl || "";
    document.getElementById("product-modal").classList.add("active");
  },

  closeModal() {
    document.getElementById("product-modal").classList.remove("active");
  },

  async saveProduct(event) {
    if (event) event.preventDefault();

    const id = document.getElementById("prod-id").value;
    const body = {
      name: document.getElementById("prod-name").value.trim(),
      description: document.getElementById("prod-desc").value.trim(),
      price: parseFloat(document.getElementById("prod-price").value),
      category: document.getElementById("prod-cat").value,
      stock: parseInt(document.getElementById("prod-stock").value, 10),
      imageUrl: document.getElementById("prod-img").value.trim()
    };

    if (!body.name || isNaN(body.price) || body.price <= 0 || isNaN(body.stock)) {
      showToast("Please enter valid name, price and stock values.", "error");
      return;
    }

    let res;
    if (id) {
      // Update existing
      res = await api.put(`/products/${id}`, body);
    } else {
      // Create new
      res = await api.post("/products", body);
    }

    if (res.success) {
      showToast(res.message || "Product saved successfully!", "success");
      this.closeModal();
      this.loadMyProducts();
    } else {
      showToast(res.message || "Failed to save product.", "error");
    }
  },

  async deleteProduct(productId) {
    if (!confirm("Are you sure you want to delete this product?")) return;

    const res = await api.delete(`/products/${productId}`);
    if (res.success) {
      showToast("Product deleted.", "info");
      this.loadMyProducts();
    } else {
      showToast(res.message || "Failed to delete product.", "error");
    }
  },

  async updateOrderStatus(orderId, newStatus) {
    const res = await api.put(`/orders/${orderId}/status`, { status: newStatus });
    if (res.success) {
      showToast(`Order status updated to ${newStatus}`, "success");
    } else {
      showToast(res.message || "Failed to update order status.", "error");
    }
  }
};
