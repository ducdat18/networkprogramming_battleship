# Dockerfile for Battleship Game Server
# Multi-stage build for smaller image size

# Stage 1: Build
FROM ubuntu:22.04 AS builder

# Set working directory
WORKDIR /app

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    pkg-config \
    libsqlite3-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY common/ ./common/
COPY server/ ./server/
COPY Makefile .

# Build server
RUN make server

# Stage 2: Runtime
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    sqlite3 \
    libssl3 \
    netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy built binary from builder
COPY --from=builder /app/bin/battleship_server /app/battleship_server

# Create data directory for database
RUN mkdir -p /app/data

# Expose server port
EXPOSE 8888

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD nc -z localhost 8888 || exit 1

# Run server
CMD ["./battleship_server"]
