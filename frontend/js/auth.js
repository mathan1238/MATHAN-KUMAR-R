/**
 * Mathan Mart Auth Service (Frontend)
 * Manages user sessions, registration, login, logout, and dynamic header state.
 */

const auth = {
  isLoggedIn() {
    return !!api.getToken() && !!api.getUser();
  },

  getCurrentUser() {
    return api.getUser();
  },

  hasRole(role) {
    const user = this.getCurrentUser();
    return user && user.role === role;
  },

  async register(name, email, password, role) {
    const res = await api.post("/auth/register", { name, email, password, role });
    if (res.success && res.data) {
      api.setToken(res.data.token);
      api.setUser(res.data.user);
      showToast("Account created successfully! Welcome to Mathan Mart.", "success");
      setTimeout(() => {
        if (res.data.user.role === "seller") {
          window.location.href = "seller-dashboard.html";
        } else {
          window.location.href = "index.html";
        }
      }, 1000);
      return true;
    } else {
      showToast(res.message || "Registration failed.", "error");
      return false;
    }
  },

  async login(email, password) {
    const res = await api.post("/auth/login", { email, password });
    if (res.success && res.data) {
      api.setToken(res.data.token);
      api.setUser(res.data.user);
      showToast(`Welcome back, ${res.data.user.name}!`, "success");
      setTimeout(() => {
        if (res.data.user.role === "admin") {
          window.location.href = "admin-dashboard.html";
        } else if (res.data.user.role === "seller") {
          window.location.href = "seller-dashboard.html";
        } else {
          window.location.href = "index.html";
        }
      }, 800);
      return true;
    } else {
      showToast(res.message || "Invalid credentials.", "error");
      return false;
    }
  },

  async logout() {
    await api.post("/auth/logout", {});
    api.setToken(null);
    api.setUser(null);
    showToast("You have been logged out.", "info");
    setTimeout(() => {
      window.location.href = "login.html";
    }, 800);
  },

  async updateNav() {
    const actionsContainer = document.getElementById("header-actions");
    if (!actionsContainer) return;

    const user = this.getCurrentUser();

    if (user) {
      let roleLinks = "";
      if (user.role === "seller") {
        roleLinks = `<a href="seller-dashboard.html" class="header-link">📊 Seller Dashboard</a>`;
      } else if (user.role === "admin") {
        roleLinks = `<a href="admin-dashboard.html" class="header-link">🛡️ Admin Dashboard</a>`;
      } else {
        roleLinks = `<a href="orders.html" class="header-link">📦 My Orders</a>`;
      }

      actionsContainer.innerHTML = `
        ${roleLinks}
        <div class="header-link user-profile" style="cursor: default;">
          <span>👤</span>
          <span>${user.name} <small style="color: var(--primary); font-weight: 700;">(${user.role.toUpperCase()})</small></span>
        </div>
        ${user.role === 'buyer' ? `
          <a href="cart.html" class="header-link cart-btn">
            🛒 Cart <span id="cart-count" class="cart-badge">0</span>
          </a>
        ` : ''}
        <button onclick="auth.logout()" class="btn btn-secondary btn-sm" style="margin-left: 8px;">
          Logout
        </button>
      `;

      if (user.role === 'buyer') {
        this.updateCartBadge();
      }
    } else {
      actionsContainer.innerHTML = `
        <a href="login.html" class="header-link">👤 Sign In</a>
        <a href="register.html" class="btn btn-primary btn-sm">Register</a>
        <a href="cart.html" class="header-link cart-btn">
          🛒 Cart <span id="cart-count" class="cart-badge">0</span>
        </a>
      `;
    }
  },

  async updateCartBadge() {
    const badge = document.getElementById("cart-count");
    if (!badge) return;

    const user = this.getCurrentUser();
    if (!user || user.role !== "buyer") {
      badge.textContent = "0";
      return;
    }

    const res = await api.get("/cart");
    if (res.success && res.data) {
      badge.textContent = res.data.itemCount || 0;
    }
  }
};

document.addEventListener("DOMContentLoaded", () => {
  auth.updateNav();
});
