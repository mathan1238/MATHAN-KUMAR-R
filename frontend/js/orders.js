/**
 * Mathan Mart Orders Client
 * Displays buyer order history, item breakdowns, status tracking, and review links.
 */

const orders = {
  formatINR(amount) {
    return new Intl.NumberFormat('en-IN', {
      style: 'currency',
      currency: 'INR',
      maximumFractionDigits: 0
    }).format(amount);
  },

  async loadOrders() {
    const user = auth.getCurrentUser();
    if (!user || user.role !== "buyer") {
      window.location.href = "login.html";
      return;
    }

    const container = document.getElementById("orders-list");
    if (!container) return;

    container.innerHTML = `<p style="text-align: center; padding: 40px; color: var(--text-muted);">Loading your order history...</p>`;

    const res = await api.get("/orders");

    if (!res.success || !res.data || res.data.length === 0) {
      container.innerHTML = `
        <div style="text-align: center; padding: 60px 20px; background: white; border-radius: 12px; border: 1px dashed var(--border-color);">
          <span style="font-size: 40px;">📦</span>
          <h2 style="margin: 16px 0 8px 0;">No Orders Placed Yet</h2>
          <p style="color: var(--text-muted); margin-bottom: 20px;">You haven't made any purchases with us yet.</p>
          <a href="products.html" class="btn btn-primary">Discover Products</a>
        </div>
      `;
      return;
    }

    container.innerHTML = res.data.map(order => {
      const itemsHtml = (order.items || []).map(item => `
        <div style="display: flex; gap: 16px; padding: 12px 0; border-bottom: 1px solid #f1f5f9; align-items: center;">
          <img src="${item.productImageUrl || 'https://images.unsplash.com/photo-1526170375885-4d8ecf77b99f?w=200'}" 
               alt="${item.productName}" 
               style="width: 60px; height: 60px; object-fit: cover; border-radius: 8px;">
          <div style="flex: 1;">
            <a href="product-details.html?id=${item.productId}" style="font-weight: 700; color: var(--text-main);">
              ${item.productName}
            </a>
            <div style="font-size: 13px; color: var(--text-muted); margin-top: 2px;">
              Seller: ${item.sellerName || 'Verified Merchant'} | Qty: ${item.quantity} × ${this.formatINR(item.price)}
            </div>
          </div>
          <div style="text-align: right;">
            <div style="font-weight: 700;">${this.formatINR(item.subtotal)}</div>
            <a href="product-details.html?id=${item.productId}#reviews" 
               class="btn btn-secondary btn-sm" style="margin-top: 6px; display: inline-block;">
              ⭐ Write Review
            </a>
          </div>
        </div>
      `).join("");

      const statusClass = `status-${order.status.toLowerCase()}`;

      return `
        <div style="background: white; border-radius: var(--radius); border: 1px solid var(--border-color); box-shadow: var(--shadow); margin-bottom: 24px; overflow: hidden;">
          <div style="background: #f8fafc; padding: 16px 20px; border-bottom: 1px solid var(--border-color); display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 12px;">
            <div>
              <span style="font-size: 12px; color: var(--text-muted); text-transform: uppercase; font-weight: 600;">Order ID</span>
              <div style="font-weight: 800; font-size: 16px; color: var(--secondary);">#MM-${order.id}</div>
            </div>
            <div>
              <span style="font-size: 12px; color: var(--text-muted); text-transform: uppercase; font-weight: 600;">Date Placed</span>
              <div style="font-size: 14px; font-weight: 500;">${order.createdAt}</div>
            </div>
            <div>
              <span style="font-size: 12px; color: var(--text-muted); text-transform: uppercase; font-weight: 600;">Total Amount</span>
              <div style="font-weight: 800; font-size: 16px; color: var(--primary);">${this.formatINR(order.totalAmount)}</div>
            </div>
            <div>
              <span class="status-badge ${statusClass}">${order.status}</span>
            </div>
          </div>
          <div style="padding: 16px 20px;">
            ${itemsHtml}
          </div>
        </div>
      `;
    }).join("");
  }
};
