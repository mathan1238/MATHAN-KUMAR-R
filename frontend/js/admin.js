/**
 * Mathan Mart Admin Dashboard Handler
 * Platform statistics, user management, global products oversight, and order logs.
 */

const admin = {
  formatINR(amount) {
    return new Intl.NumberFormat('en-IN', {
      style: 'currency',
      currency: 'INR',
      maximumFractionDigits: 0
    }).format(amount);
  },

  async init() {
    const user = auth.getCurrentUser();
    if (!user || user.role !== "admin") {
      showToast("Access restricted: Administrator privileges required.", "error");
      setTimeout(() => {
        window.location.href = "login.html";
      }, 1000);
      return;
    }

    const adminNameEl = document.getElementById("admin-name");
    if (adminNameEl) adminNameEl.textContent = user.name;

    await this.loadStats();
    await this.loadProducts();
    await this.loadUsers();
    await this.loadOrders();
  },

  async loadStats() {
    const res = await api.get("/admin/stats");
    if (res.success && res.data) {
      const s = res.data;
      document.getElementById("stat-users").textContent = s.totalUsers || 0;
      document.getElementById("stat-sellers").textContent = s.totalSellers || 0;
      document.getElementById("stat-buyers").textContent = s.totalBuyers || 0;
      document.getElementById("stat-products").textContent = s.totalProducts || 0;
      document.getElementById("stat-orders").textContent = s.totalOrders || 0;
      document.getElementById("stat-sales").textContent = this.formatINR(s.totalSales || 0);
    }
  },

  async loadUsers() {
    const tbody = document.getElementById("admin-users-body");
    if (!tbody) return;

    const res = await api.get("/admin/users");
    if (!res.success || !res.data || res.data.length === 0) {
      tbody.innerHTML = `<tr><td colspan="5" style="text-align: center; padding: 20px; color: var(--text-muted);">No users found.</td></tr>`;
      return;
    }

    tbody.innerHTML = res.data.map(u => `
      <tr>
        <td>#${u.id}</td>
        <td><strong>${u.name}</strong></td>
        <td>${u.email}</td>
        <td>
          <span class="status-badge" style="background: ${u.role === 'admin' ? '#fee2e2' : u.role === 'seller' ? '#e0e7ff' : '#f0fdf4'}; color: ${u.role === 'admin' ? '#b91c1c' : u.role === 'seller' ? '#4338ca' : '#15803d'};">
            ${u.role.toUpperCase()}
          </span>
        </td>
        <td>${u.createdAt}</td>
      </tr>
    `).join("");
  },

  async loadProducts() {
    const tbody = document.getElementById("admin-products-body");
    if (!tbody) return;

    const res = await api.get("/admin/products");
    if (!res.success || !res.data || res.data.length === 0) {
      tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; padding: 20px; color: var(--text-muted);">No products found.</td></tr>`;
      return;
    }

    tbody.innerHTML = res.data.map(p => `
      <tr>
        <td>#${p.id}</td>
        <td><strong>${p.name}</strong></td>
        <td>${p.sellerName || `Seller #${p.sellerId}`}</td>
        <td>${p.category}</td>
        <td><strong>${this.formatINR(p.price)}</strong></td>
        <td>${p.stock}</td>
        <td>
          <button onclick="admin.deleteProduct(${p.id})" 
                  class="btn btn-sm" style="background: #fee2e2; color: #b91c1c; border: none;">
            Remove Product
          </button>
        </td>
      </tr>
    `).join("");
  },

  async loadOrders() {
    const tbody = document.getElementById("admin-orders-body");
    if (!tbody) return;

    const res = await api.get("/admin/orders");
    if (!res.success || !res.data || res.data.length === 0) {
      tbody.innerHTML = `<tr><td colspan="6" style="text-align: center; padding: 20px; color: var(--text-muted);">No orders placed yet.</td></tr>`;
      return;
    }

    tbody.innerHTML = res.data.map(o => `
      <tr>
        <td>#MM-${o.id}</td>
        <td>${o.buyerName} <small style="color: var(--text-muted);">(${o.buyerEmail})</small></td>
        <td><strong>${this.formatINR(o.totalAmount)}</strong></td>
        <td><span class="status-badge status-${o.status.toLowerCase()}">${o.status}</span></td>
        <td>${o.createdAt}</td>
        <td>
          <select onchange="admin.updateOrderStatus(${o.id}, this.value)" 
                  style="padding: 4px 8px; border-radius: 6px; border: 1px solid var(--border-color); font-size: 13px;">
            <option value="Pending" ${o.status === 'Pending' ? 'selected' : ''}>Pending</option>
            <option value="Processing" ${o.status === 'Processing' ? 'selected' : ''}>Processing</option>
            <option value="Shipped" ${o.status === 'Shipped' ? 'selected' : ''}>Shipped</option>
            <option value="Delivered" ${o.status === 'Delivered' ? 'selected' : ''}>Delivered</option>
            <option value="Cancelled" ${o.status === 'Cancelled' ? 'selected' : ''}>Cancelled</option>
          </select>
        </td>
      </tr>
    `).join("");
  },

  async deleteProduct(productId) {
    if (!confirm("Are you sure you want to remove this product as Administrator?")) return;

    const res = await api.delete(`/admin/products/${productId}`);
    if (res.success) {
      showToast("Product successfully removed.", "info");
      this.loadProducts();
      this.loadStats();
    } else {
      showToast(res.message || "Failed to remove product.", "error");
    }
  },

  async updateOrderStatus(orderId, newStatus) {
    const res = await api.put(`/orders/${orderId}/status`, { status: newStatus });
    if (res.success) {
      showToast(`Order #MM-${orderId} status set to ${newStatus}`, "success");
      this.loadOrders();
    } else {
      showToast(res.message || "Failed to update order status.", "error");
    }
  },

  switchTab(tabName) {
    document.querySelectorAll(".tab-content").forEach(el => el.style.display = "none");
    document.querySelectorAll(".tab-btn").forEach(el => el.classList.remove("active"));

    const target = document.getElementById(`tab-${tabName}`);
    if (target) target.style.display = "block";

    const btn = document.getElementById(`tab-btn-${tabName}`);
    if (btn) btn.classList.add("active");
  }
};
