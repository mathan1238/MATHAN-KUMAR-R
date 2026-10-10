/**
 * Mathan Mart Checkout Handler
 * Communicates with POST /api/orders/checkout for server-side transactional order placement.
 */

const checkout = {
  formatINR(amount) {
    return new Intl.NumberFormat('en-IN', {
      style: 'currency',
      currency: 'INR',
      maximumFractionDigits: 0
    }).format(amount);
  },

  async init() {
    const user = auth.getCurrentUser();
    if (!user || user.role !== "buyer") {
      window.location.href = "login.html";
      return;
    }

    const res = await api.get("/cart");
    if (!res.success || !res.data || !res.data.items || res.data.items.length === 0) {
      showToast("Your cart is empty. Please add items before checking out.", "error");
      setTimeout(() => {
        window.location.href = "products.html";
      }, 1500);
      return;
    }

    const { items, totalAmount } = res.data;

    const summaryList = document.getElementById("checkout-items");
    if (summaryList) {
      summaryList.innerHTML = items.map(item => `
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; font-size: 14px;">
          <div>
            <strong style="color: var(--text-main);">${item.name}</strong>
            <div style="color: var(--text-muted); font-size: 12px;">Qty: ${item.quantity} × ${this.formatINR(item.price)}</div>
          </div>
          <span style="font-weight: 700;">${this.formatINR(item.subtotal)}</span>
        </div>
      `).join("");
    }

    const totalEl = document.getElementById("checkout-total");
    if (totalEl) {
      totalEl.textContent = this.formatINR(totalAmount);
    }
  },

  async placeOrder(event) {
    if (event) event.preventDefault();

    const nameInput = document.getElementById("ship-name");
    const addressInput = document.getElementById("ship-address");
    const cityInput = document.getElementById("ship-city");
    const zipInput = document.getElementById("ship-zip");

    if (!nameInput.value.trim() || !addressInput.value.trim() || !cityInput.value.trim() || !zipInput.value.trim()) {
      showToast("Please fill in all required shipping address fields.", "error");
      return;
    }

    const placeBtn = document.getElementById("btn-place-order");
    if (placeBtn) {
      placeBtn.disabled = true;
      placeBtn.textContent = "Processing Order...";
    }

    // Call backend transactional checkout endpoint
    const res = await api.post("/orders/checkout", {});

    if (res.success && res.data) {
      showToast("Order placed successfully!", "success");
      auth.updateCartBadge();

      // Show Success Screen
      const container = document.getElementById("checkout-view");
      if (container) {
        container.innerHTML = `
          <div style="background: white; padding: 50px 30px; border-radius: 16px; border: 1px solid var(--border-color); text-align: center; max-width: 600px; margin: 40px auto; box-shadow: var(--shadow-hover);">
            <div style="width: 70px; height: 70px; background: #dcfce7; border-radius: 50%; display: flex; align-items: center; justify-content: center; margin: 0 auto 20px auto; font-size: 36px; color: var(--success);">
              ✓
            </div>
            <h1 style="font-size: 28px; font-weight: 800; color: var(--secondary); margin-bottom: 10px;">Order Successful!</h1>
            <p style="color: var(--text-muted); font-size: 15px; margin-bottom: 24px;">Thank you for shopping with Mathan Mart. Your order has been placed and is being prepared.</p>
            
            <div style="background: #f8fafc; padding: 20px; border-radius: 12px; margin-bottom: 30px; text-align: left; font-size: 14px;">
              <div style="display: flex; justify-content: space-between; margin-bottom: 8px;">
                <span style="color: var(--text-muted);">Order Reference ID:</span>
                <strong>#MM-${res.data.orderId}</strong>
              </div>
              <div style="display: flex; justify-content: space-between; margin-bottom: 8px;">
                <span style="color: var(--text-muted);">Total Paid:</span>
                <strong style="color: var(--primary); font-size: 16px;">${this.formatINR(res.data.totalAmount)}</strong>
              </div>
              <div style="display: flex; justify-content: space-between;">
                <span style="color: var(--text-muted);">Payment Method:</span>
                <span>Cash on Delivery (Verified)</span>
              </div>
            </div>

            <div style="display: flex; gap: 14px; justify-content: center;">
              <a href="orders.html" class="btn btn-primary" style="padding: 12px 24px;">View My Orders</a>
              <a href="products.html" class="btn btn-secondary" style="padding: 12px 24px;">Continue Shopping</a>
            </div>
          </div>
        `;
      }
    } else {
      if (placeBtn) {
        placeBtn.disabled = false;
        placeBtn.textContent = "Confirm Order & Pay";
      }
      showToast(res.message || "Failed to complete order. Please check item stock.", "error");
    }
  }
};
