# ==============================================================================
# Mathan Mart - Multi-Stage Production Dockerfile for Render Deployment
# ==============================================================================

# ------------------------------------------------------------------------------
# Stage 1: Build Environment
# ------------------------------------------------------------------------------
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build essentials and development libraries
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libpq-dev \
    libssl-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libbrotli-dev \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# Build and install Drogon with PostgreSQL support
WORKDIR /tmp/drogon-src
RUN git clone --depth 1 https://github.com/drogonframework/drogon.git . \
    && git submodule update --init \
    && mkdir build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_POSTGRESQL=ON \
        -DBUILD_SQLITE=OFF \
        -DBUILD_MYSQL=OFF \
        -DBUILD_REDIS=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF \
    && cmake --build . -j$(nproc) \
    && cmake --install . \
    && rm -rf /tmp/drogon-src

# Copy project source code
WORKDIR /app
COPY . .

# Build Mathan Mart application
RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && cmake --build . -j$(nproc)

# ------------------------------------------------------------------------------
# Stage 2: Minimal Runtime Environment
# ------------------------------------------------------------------------------
FROM ubuntu:22.04 AS runner

ENV DEBIAN_FRONTEND=noninteractive

# Install only the runtime shared libraries
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    libssl3 \
    libjsoncpp25 \
    zlib1g \
    libbrotli1 \
    libuuid1 \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy compiled backend binary and frontend assets from builder stage
COPY --from=builder /app/build/backend/MathanMartBackend /app/MathanMartBackend
COPY --from=builder /app/frontend /app/frontend

# Default Render port
ENV PORT=10000
EXPOSE 10000

# Execute server binary
CMD ["/app/MathanMartBackend"]
