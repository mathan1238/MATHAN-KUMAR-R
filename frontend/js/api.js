/**
 * Mathan Mart API Client
 * Centralized Fetch wrapper with automatic Bearer token injection,
 * error parsing, and UI toast notifications.
 */

const API_BASE = "/api";

function showToast(message, type = "info") {
  let container = document.getElementById("toast-container");
  if (!container) {
    container = document.createElement("div");
    container.id = "toast-container";
    document.body.appendChild(container);
  }

  const toast = document.createElement("div");
  toast.className = `toast toast-${type}`;
  toast.innerHTML = `
    <span>${type === 'success' ? '✅' : type === 'error' ? '❌' : 'ℹ️'}</span>
    <span>${message}</span>
  `;

  container.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = "0";
    toast.style.transform = "translateX(100%)";
    toast.style.transition = "all 0.3s ease";
    setTimeout(() => toast.remove(), 300);
  }, 3500);
}

const api = {
  getToken() {
    return localStorage.getItem("mathan_mart_token") || "";
  },

  setToken(token) {
    if (token) {
      localStorage.setItem("mathan_mart_token", token);
    } else {
      localStorage.removeItem("mathan_mart_token");
    }
  },

  getUser() {
    try {
      const u = localStorage.getItem("mathan_mart_user");
      return u ? JSON.parse(u) : null;
    } catch {
      return null;
    }
  },

  setUser(user) {
    if (user) {
      localStorage.setItem("mathan_mart_user", JSON.stringify(user));
    } else {
      localStorage.removeItem("mathan_mart_user");
    }
  },

  async request(endpoint, options = {}) {
    const url = endpoint.startsWith("http") ? endpoint : `${API_BASE}${endpoint}`;
    
    const headers = {
      "Content-Type": "application/json",
      ...(options.headers || {})
    };

    const token = this.getToken();
    if (token) {
      headers["Authorization"] = `Bearer ${token}`;
    }

    try {
      const response = await fetch(url, {
        ...options,
        headers
      });

      let data;
      const contentType = response.headers.get("content-type") || "";
      if (contentType.includes("application/json")) {
        data = await response.json();
      } else {
        const text = await response.text();
        try {
          data = JSON.parse(text);
        } catch {
          data = { success: response.ok, message: text };
        }
      }

      if (!response.ok) {
        if (response.status === 401) {
          // Token expired or invalid
          this.setToken(null);
          this.setUser(null);
        }
        const errorMsg = data && data.message ? data.message : `HTTP Error ${response.status}`;
        return {
          success: false,
          status: response.status,
          message: errorMsg,
          data: null
        };
      }

      return data;
    } catch (err) {
      console.error("[API Network Error]", err);
      showToast("Network connection error. Please verify backend server is running.", "error");
      return {
        success: false,
        status: 0,
        message: "Failed to connect to server.",
        data: null
      };
    }
  },

  get(endpoint) {
    return this.request(endpoint, { method: "GET" });
  },

  post(endpoint, body) {
    return this.request(endpoint, {
      method: "POST",
      body: JSON.stringify(body)
    });
  },

  put(endpoint, body) {
    return this.request(endpoint, {
      method: "PUT",
      body: JSON.stringify(body)
    });
  },

  delete(endpoint) {
    return this.request(endpoint, { method: "DELETE" });
  }
};
