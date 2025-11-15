#!/bin/bash
# Script to run integration tests with automatic server startup/shutdown

set -e

SERVER_BIN="./bin/battleship_server"
SERVER_PORT=8888
SERVER_PID=""
USE_DOCKER=false

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --docker)
            USE_DOCKER=true
            shift
            ;;
        *)
            shift
            ;;
    esac
done

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Integration Test Runner${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""

# Cleanup function
cleanup() {
    echo ""
    echo -e "${YELLOW}Cleaning up...${NC}"

    if [ "$USE_DOCKER" = true ]; then
        echo "Stopping Docker container..."
        docker compose down > /dev/null 2>&1 || true
    else
        if [ ! -z "$SERVER_PID" ]; then
            echo "Stopping server (PID: $SERVER_PID)..."
            kill $SERVER_PID 2>/dev/null || true
            wait $SERVER_PID 2>/dev/null || true
        fi
    fi

    echo -e "${GREEN}Cleanup complete${NC}"
}

# Set trap to cleanup on exit
trap cleanup EXIT INT TERM

# Check if server binary exists
if [ ! -f "$SERVER_BIN" ]; then
    echo -e "${RED}Error: Server binary not found at $SERVER_BIN${NC}"
    echo "Please build the server first: make server"
    exit 1
fi

# Start server
if [ "$USE_DOCKER" = true ]; then
    echo -e "${CYAN}[1/5] Building Docker image...${NC}"
    docker compose build || {
        echo -e "${RED}Error: Failed to build Docker image${NC}"
        exit 1
    }
    echo ""

    echo -e "${CYAN}[2/5] Starting server in Docker...${NC}"
    docker compose up -d
    echo "Waiting for server to start..."
    sleep 3
else
    echo -e "${CYAN}[1/4] Starting server...${NC}"
    $SERVER_BIN > /tmp/battleship_server.log 2>&1 &
    SERVER_PID=$!
    echo "Server started (PID: $SERVER_PID)"

    # Wait for server to start
    echo "Waiting for server to start..."
    sleep 2

    # Check if server is running
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${RED}Error: Server failed to start${NC}"
        cat /tmp/battleship_server.log
        exit 1
    fi
fi

# Check if server is listening
if [ "$USE_DOCKER" = true ]; then
    echo -e "${CYAN}[3/5] Checking server status...${NC}"
else
    echo -e "${CYAN}[2/4] Checking server status...${NC}"
fi

if nc -z localhost $SERVER_PORT 2>/dev/null; then
    echo -e "${GREEN}✓ Server is listening on port $SERVER_PORT${NC}"
else
    echo -e "${RED}✗ Server is not responding on port $SERVER_PORT${NC}"
    if [ "$USE_DOCKER" = false ]; then
        echo "Server log:"
        cat /tmp/battleship_server.log
    fi
    exit 1
fi

# Build integration tests
if [ "$USE_DOCKER" = true ]; then
    echo -e "${CYAN}[4/5] Building integration tests...${NC}"
else
    echo -e "${CYAN}[3/4] Building integration tests...${NC}"
fi

make bin/test_client_server bin/test_authentication > /dev/null 2>&1 || {
    echo -e "${RED}Error: Failed to build integration tests${NC}"
    exit 1
}

# Run integration tests
echo ""
if [ "$USE_DOCKER" = true ]; then
    echo -e "${CYAN}[5/5] Running integration tests...${NC}"
else
    echo -e "${CYAN}[4/4] Running integration tests...${NC}"
fi
echo ""

# Run client-server tests
echo -e "${YELLOW}Running client-server tests...${NC}"
./bin/test_client_server
echo ""

# Run authentication tests (with DISABLED tests enabled)
echo -e "${YELLOW}Running authentication tests...${NC}"
./bin/test_authentication --gtest_also_run_disabled_tests

# Success
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  ✓ All Integration Tests Passed!${NC}"
echo -e "${GREEN}========================================${NC}"
