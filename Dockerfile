# ==============================================================================
# Mathan Mart - Multi-Stage Production Dockerfile for Render Deployment
# Memory-Optimized for Low-RAM Environments (prevents OOM on Render builds)
# ==============================================================================

# ------------------------------------------------------------------------------
# Stage 1: Build Environment
# ------------------------------------------------------------------------------
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
# Limit CMake build parallelism globally to prevent GCC compiler memory exhaustion
ENV CMAKE_BUILD_PARALLEL_LEVEL=2

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

# Build and install Drogon with PostgreSQL support (memory-optimized)
WORKDIR /tmp/drogon-src
RUN git clone --depth 1 --branch v1.9.13 https://github.com/drogonframework/drogon.git . \
    && git submodule update --init \
    && mkdir build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-O2 -pipe -g0 -fno-var-tracking-assignments" \
        -DBUILD_POSTGRESQL=ON \
        -DBUILD_SQLITE=OFF \
        -DBUILD_MYSQL=OFF \
        -DBUILD_REDIS=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF \
        -DBUILD_CTL=OFF \
        -DBUILD_DOC=OFF \
    && cmake --build . -j2 \
    && cmake --install . \
    && rm -rf /tmp/drogon-src

# Copy project source code
WORKDIR /app
COPY . .

# Build Mathan Mart application with capped parallelism
RUN mkdir -p build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-O2 -pipe -g0" \
    && cmake --build . -j2

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

# Copy any Drogon/Trantor installed shared libraries and refresh linker cache
COPY --from=builder /usr/local/lib /usr/local/lib
RUN ldconfig

WORKDIR /app

# Copy compiled backend binary and frontend assets from builder stage
COPY --from=builder /app/build/backend/MathanMartBackend /app/MathanMartBackend
COPY --from=builder /app/frontend /app/frontend

# Default Render port
ENV PORT=10000
EXPOSE 10000

# Execute server binary
CMD ["/app/MathanMartBackend"]
