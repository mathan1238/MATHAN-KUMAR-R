/**
 * Mathan Mart Cart Management
 * Fetches cart contents from REST API, updates quantities, and calculates totals on backend.
 */

const cart = {
  formatINR(amount) {
    return new Intl.NumberFormat('en-IN', {
      style: 'currency',
      currency: 'INR',
      maximumFractionDigits: 0
    }).format(amount);
  },

  async loadCart() {
    const user = auth.getCurrentUser();
    if (!user) {
      document.getElementById("cart-content").innerHTML = `
        <div style="text-align: center; padding: 60px 20px; background: white; border-radius: 12px;">
          <h2>Please Sign In</h2>
          <p style="color: var(--text-muted); margin: 12px 0 24px 0;">You need to be logged in as a buyer to view your shopping cart.</p>
          <a href="login.html" class="btn btn-primary">Sign In Now</a>
        </div>
      `;
      return;
    }

    if (user.role !== "buyer") {
      document.getElementById("cart-content").innerHTML = `
        <div style="text-align: center; padding: 60px 20px; background: white; border-radius: 12px;">
          <h2>Buyer Access Only</h2>
          <p style="color: var(--text-muted); margin: 12px 0 24px 0;">Your account role is <strong>${user.role}</strong>. Only buyer accounts can maintain a shopping cart.</p>
          <a href="index.html" class="btn btn-secondary">Go to Home</a>
        </div>
      `;
      return;
    }

    const res = await api.get("/cart");
    const container = document.getElementById("cart-content");

    if (!res.success || !res.data || !res.data.items || res.data.items.length === 0) {
      container.innerHTML = `
        <div style="text-align: center; padding: 80px 20px; background: white; border-radius: 12px; border: 1px dashed var(--border-color);">
          <span style="font-size: 50px;">🛒</span>
          <h2 style="margin: 16px 0 8px 0;">Your Cart is Empty</h2>
          <p style="color: var(--text-muted); margin-bottom: 24px;">Looks like you haven't added anything to your cart yet.</p>
          <a href="products.html" class="btn btn-primary">Start Shopping</a>
        </div>
      `;
      auth.updateCartBadge();
      return;
    }

    const { items, totalAmount, itemCount } = res.data;

    let itemsHtml = items.map(item => `
      <div class="cart-item" style="display: flex; gap: 20px; padding: 20px; border-bottom: 1px solid var(--border-color); align-items: center;">
        <img src="${item.imageUrl || 'https://images.unsplash.com/photo-1526170375885-4d8ecf77b99f?w=200'}" 
             alt="${item.name}" 
             style="width: 80px; height: 80px; object-fit: cover; border-radius: 8px;">
        <div style="flex: 1;">
          <a href="product-details.html?id=${item.productId}" style="font-weight: 700; font-size: 16px; color: var(--text-main);">
            ${item.name}
          </a>
          <div style="font-size: 13px; color: var(--text-muted); margin: 4px 0;">Category: ${item.category}</div>
          <div style="font-size: 15px; font-weight: 700; color: var(--primary);">${this.formatINR(item.price)} each</div>
        </div>
        <div style="display: flex; align-items: center; gap: 8px;">
          <button onclick="cart.changeQty(${item.productId}, ${item.quantity - 1})" 
                  class="btn btn-secondary btn-sm" style="padding: 4px 10px;">-</button>
          <span style="font-weight: 700; min-width: 24px; text-align: center;">${item.quantity}</span>
          <button onclick="cart.changeQty(${item.productId}, ${item.quantity + 1})" 
                  class="btn btn-secondary btn-sm" style="padding: 4px 10px;"
                  ${item.quantity >= item.stock ? 'disabled' : ''}>+</button>
        </div>
        <div style="min-width: 100px; text-align: right;">
          <div style="font-weight: 800; font-size: 16px;">${this.formatINR(item.subtotal)}</div>
          <button onclick="cart.removeItem(${item.productId})" 
                  style="background: transparent; border: none; color: var(--danger); font-size: 12px; margin-top: 6px; cursor: pointer;">
            Remove
          </button>
        </div>
      </div>
    `).join("");

    container.innerHTML = `
      <div class="cart-container" style="display: grid; grid-template-columns: 2fr 1fr; gap: 30px;">
        <div style="background: white; border-radius: var(--radius); border: 1px solid var(--border-color); box-shadow: var(--shadow);">
          <div style="padding: 18px 20px; border-bottom: 1px solid var(--border-color); display: flex; justify-content: space-between; align-items: center;">
            <h2 style="font-size: 18px; font-weight: 800;">Shopping Items (${itemCount})</h2>
            <button onclick="cart.clear()" style="background: transparent; border: none; color: var(--danger); font-size: 13px; font-weight: 600; cursor: pointer;">
              Clear All
            </button>
          </div>
          <div>${itemsHtml}</div>
        </div>

        <div>
          <div style="background: white; padding: 24px; border-radius: var(--radius); border: 1px solid var(--border-color); box-shadow: var(--shadow); position: sticky; top: 100px;">
            <h3 style="font-size: 18px; font-weight: 800; margin-bottom: 20px;">Order Summary</h3>
            <div style="display: flex; justify-content: space-between; margin-bottom: 12px; font-size: 14px;">
              <span style="color: var(--text-muted);">Items Total (${itemCount})</span>
              <span style="font-weight: 600;">${this.formatINR(totalAmount)}</span>
            </div>
            <div style="display: flex; justify-content: space-between; margin-bottom: 12px; font-size: 14px;">
              <span style="color: var(--text-muted);">Shipping</span>
              <span style="color: var(--success); font-weight: 700;">FREE</span>
            </div>
            <hr style="border: none; border-top: 1px solid var(--border-color); margin: 16px 0;">
            <div style="display: flex; justify-content: space-between; margin-bottom: 24px;">
              <span style="font-size: 16px; font-weight: 700;">Grand Total</span>
              <span style="font-size: 22px; font-weight: 800; color: var(--primary);">${this.formatINR(totalAmount)}</span>
            </div>
            <a href="checkout.html" class="btn btn-primary btn-block" style="padding: 14px; font-size: 15px;">
              Proceed to Checkout →
            </a>
          </div>
        </div>
      </div>
    `;

    auth.updateCartBadge();
  },

  async changeQty(productId, newQty) {
    if (newQty <= 0) {
      await this.removeItem(productId);
      return;
    }

    const res = await api.put(`/cart/${productId}`, { quantity: newQty });
    if (res.success) {
      this.loadCart();
    } else {
      showToast(res.message || "Failed to update quantity.", "error");
    }
  },

  async removeItem(productId) {
    const res = await api.delete(`/cart/${productId}`);
    if (res.success) {
      showToast("Item removed from cart.", "info");
      this.loadCart();
    } else {
      showToast(res.message || "Failed to remove item.", "error");
    }
  },

  async clear() {
    if (!confirm("Are you sure you want to empty your shopping cart?")) return;
    const res = await api.delete("/cart");
    if (res.success) {
      showToast("Cart cleared.", "info");
      this.loadCart();
    } else {
      showToast(res.message || "Failed to clear cart.", "error");
    }
  }
};
