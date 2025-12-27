#!/bin/bash
# Script to run integration tests with automatic server startup/shutdown

set -e

SERVER_BIN="./bin/battleship_server"
SERVER_PORT=9999
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

    # Clean up WAL files
    rm -f data/battleship.db-shm data/battleship.db-wal 2>/dev/null || true

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

# Clean database BEFORE starting server for fresh test run
echo -e "${YELLOW}Cleaning database for fresh test run...${NC}"
rm -f data/battleship.db data/battleship.db-shm data/battleship.db-wal
rm -f ~/.battleship/session.txt
echo -e "${GREEN}✓ Database and session files cleaned${NC}"
echo ""

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
    sleep 5
else
    echo -e "${CYAN}[1/4] Starting server...${NC}"
    $SERVER_BIN $SERVER_PORT > /tmp/battleship_server.log 2>&1 &
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
    if [ "$USE_DOCKER" = true ]; then
        echo "Docker container status:"
        docker compose ps
        echo ""
        echo "Server logs from Docker:"
        docker compose logs server
    else
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

make bin/test_client_server bin/test_authentication bin/test_e2e_client_auth bin/test_auto_login bin/test_player_list bin/test_challenge > /dev/null 2>&1 || {
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
sleep 0.5  # Allow server to cleanup connections

# Run authentication tests (only enabled tests)
echo -e "${YELLOW}Running authentication tests...${NC}"
./bin/test_authentication
echo ""
sleep 0.5  # Allow server to cleanup connections

# Run E2E client authentication tests (only enabled tests)
echo -e "${YELLOW}Running E2E client authentication tests...${NC}"
./bin/test_e2e_client_auth
echo ""
sleep 0.5  # Allow server to cleanup connections

# Run auto-login integration tests
echo -e "${YELLOW}Running auto-login integration tests...${NC}"
./bin/test_auto_login
echo ""
sleep 0.5  # Allow server to cleanup connections

# Run player list integration tests
echo -e "${YELLOW}Running player list integration tests...${NC}"
./bin/test_player_list
echo ""
sleep 0.5  # Allow server to cleanup connections

# Run challenge integration tests
echo -e "${YELLOW}Running challenge integration tests...${NC}"
./bin/test_challenge
echo ""

# Success
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  ✓ All Integration Tests Passed!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${CYAN}Test Summary:${NC}"
echo -e "  • Client-Server: ✓"
echo -e "  • Authentication: ✓"
echo -e "  • E2E Client Auth: ✓"
echo -e "  • Auto-Login: ✓"
echo -e "  • Player List: ✓"
echo -e "  • Challenge System: ✓"
echo ""
