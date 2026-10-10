/**
 * Mathan Mart Reviews Client
 * Manages star ratings, review submissions with verified purchase enforcement, and reviews list.
 */

const reviews = {
  renderStars(rating) {
    const full = Math.round(rating);
    let stars = "";
    for (let i = 1; i <= 5; i++) {
      stars += i <= full ? "★" : "☆";
    }
    return stars;
  },

  async loadReviews(productId) {
    const container = document.getElementById("reviews-list");
    if (!container) return;

    const res = await api.get(`/products/${productId}/reviews`);
    if (!res.success || !res.data || res.data.length === 0) {
      container.innerHTML = `
        <div style="text-align: center; padding: 30px; background: #f8fafc; border-radius: 8px; color: var(--text-muted);">
          No reviews yet. Be the first to write a review after purchasing!
        </div>
      `;
      return;
    }

    container.innerHTML = res.data.map(r => `
      <div style="border-bottom: 1px solid var(--border-color); padding: 16px 0;">
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px;">
          <div>
            <strong>${r.buyerName || 'Verified Buyer'}</strong>
            <span style="color: var(--success); font-size: 12px; margin-left: 8px;">✓ Verified Purchase</span>
          </div>
          <span style="color: var(--text-muted); font-size: 12px;">${r.createdAt}</span>
        </div>
        <div style="color: #f59e0b; margin-bottom: 6px; font-size: 16px;">
          ${this.renderStars(r.rating)}
        </div>
        <p style="color: var(--text-main); font-size: 14px; line-height: 1.5;">${r.comment || 'No comment provided.'}</p>
      </div>
    `).join("");
  },

  async submitReview(event, productId) {
    if (event) event.preventDefault();

    const user = auth.getCurrentUser();
    if (!user) {
      showToast("Please log in to submit a review.", "info");
      return;
    }

    if (user.role !== "buyer") {
      showToast("Only buyers who purchased this product can leave a review.", "error");
      return;
    }

    const rating = parseInt(document.getElementById("review-rating").value, 10);
    const comment = document.getElementById("review-comment").value.trim();

    const res = await api.post(`/products/${productId}/reviews`, { rating, comment });
    if (res.success) {
      showToast("Thank you for your review!", "success");
      document.getElementById("review-comment").value = "";
      this.loadReviews(productId);
    } else {
      showToast(res.message || "Failed to submit review.", "error");
    }
  }
};
