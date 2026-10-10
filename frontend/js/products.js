/**
 * Mathan Mart Products Client
 * Renders product grids, category filtering, search, and Add to Cart logic.
 */

const products = {
  formatINR(amount) {
    return new Intl.NumberFormat('en-IN', {
      style: 'currency',
      currency: 'INR',
      maximumFractionDigits: 0
    }).format(amount);
  },

  renderStars(rating) {
    const full = Math.round(rating);
    let stars = "";
    for (let i = 1; i <= 5; i++) {
      stars += i <= full ? "★" : "☆";
    }
    return stars;
  },

  async loadProducts(containerId = "product-grid", options = {}) {
    const container = document.getElementById(containerId);
    if (!container) return;

    container.innerHTML = `
      <div style="grid-column: 1/-1; text-align: center; padding: 40px; color: var(--text-muted);">
        <p>Loading products from server...</p>
      </div>
    `;

    let url = "/products";
    const params = new URLSearchParams();
    if (options.search) params.append("search", options.search);
    if (options.category && options.category.toLowerCase() !== "all") {
      params.append("category", options.category);
    }

    const queryStr = params.toString();
    if (queryStr) url += `?${queryStr}`;

    const res = await api.get(url);

    if (!res.success || !res.data || res.data.length === 0) {
      container.innerHTML = `
        <div style="grid-column: 1/-1; text-align: center; padding: 50px; background: white; border-radius: 12px; border: 1px dashed var(--border-color);">
          <span style="font-size: 40px;">🔍</span>
          <h3 style="margin: 12px 0 6px 0;">No Products Found</h3>
          <p style="color: var(--text-muted); font-size: 14px;">Try searching for a different keyword or selecting another category.</p>
        </div>
      `;
      return;
    }

    container.innerHTML = res.data.map(p => this.createProductCard(p)).join("");
  },

  createProductCard(p) {
    const isOutOfStock = p.stock <= 0;
    const fallbackImage = "https://images.unsplash.com/photo-1526170375885-4d8ecf77b99f?w=500";
    const imageUrl = p.imageUrl || fallbackImage;

    return `
      <div class="product-card">
        <a href="product-details.html?id=${p.id}" class="product-img-wrap">
          <img src="${imageUrl}" alt="${p.name}" onerror="this.src='${fallbackImage}'">
          <span class="product-badge">${p.category}</span>
        </a>
        <div class="product-details">
          <span class="product-cat">${p.category}</span>
          <a href="product-details.html?id=${p.id}">
            <h3 class="product-title">${p.name}</h3>
          </a>
          <div class="rating-row">
            <span class="rating-stars">${this.renderStars(p.averageRating)}</span>
            <span>(${p.reviewCount || 0})</span>
          </div>
          <div class="product-price-row">
            <span class="price">${this.formatINR(p.price)}</span>
            <span class="stock-tag ${isOutOfStock ? 'out-of-stock' : 'in-stock'}">
              ${isOutOfStock ? 'Out of Stock' : `${p.stock} in stock`}
            </span>
          </div>
          <div class="card-actions">
            <a href="product-details.html?id=${p.id}" class="btn btn-secondary btn-sm" style="flex: 1;">
              Details
            </a>
            <button onclick="products.addToCart(${p.id})" 
                    class="btn btn-primary btn-sm" 
                    ${isOutOfStock ? 'disabled style="opacity: 0.5; cursor: not-allowed;"' : ''} 
                    style="flex: 1;">
              ${isOutOfStock ? 'Unavailable' : 'Add to Cart'}
            </button>
          </div>
        </div>
      </div>
    `;
  },

  async addToCart(productId, quantity = 1) {
    const user = auth.getCurrentUser();
    if (!user) {
      showToast("Please log in to add items to your cart.", "info");
      setTimeout(() => {
        window.location.href = "login.html";
      }, 1000);
      return;
    }

    if (user.role !== "buyer") {
      showToast("Only buyer accounts can purchase products.", "error");
      return;
    }

    const res = await api.post("/cart", { productId, quantity });
    if (res.success) {
      showToast("Product added to cart!", "success");
      auth.updateCartBadge();
    } else {
      showToast(res.message || "Failed to add product to cart.", "error");
    }
  }
};
