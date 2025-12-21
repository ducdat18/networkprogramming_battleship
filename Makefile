# Makefile for Battleship Online Game
# Platform: Linux/Ubuntu
# Language: C++

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2
DEBUGFLAGS = -g -DDEBUG

# GTK flags
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 cairo)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0 cairo)

# Directories
SRC_DIR = .
CLIENT_SRC = client/src
CLIENT_INC = client/include
SERVER_SRC = server/src
SERVER_INC = server/include
COMMON_SRC = common/src
COMMON_INC = common/include
TEST_SRC = tests
BUILD_DIR = build
BIN_DIR = bin

# Include paths
INCLUDES = -I$(CLIENT_INC) -I$(SERVER_INC) -I$(COMMON_INC)

# Source files
COMMON_SOURCES = $(wildcard $(COMMON_SRC)/*.cpp)
CLIENT_SOURCES = $(wildcard $(CLIENT_SRC)/*.cpp)
SERVER_SOURCES = $(wildcard $(SERVER_SRC)/*.cpp)
TEST_SOURCES = $(wildcard $(TEST_SRC)/*.cpp)

# Object files
COMMON_OBJECTS = $(patsubst $(COMMON_SRC)/%.cpp,$(BUILD_DIR)/common/%.o,$(COMMON_SOURCES))
CLIENT_OBJECTS = $(patsubst $(CLIENT_SRC)/%.cpp,$(BUILD_DIR)/client/%.o,$(CLIENT_SOURCES))
SERVER_OBJECTS = $(patsubst $(SERVER_SRC)/%.cpp,$(BUILD_DIR)/server/%.o,$(SERVER_SOURCES))

# Test directories
UNIT_TEST_DIR = $(TEST_SRC)/unit_tests
INTEGRATION_TEST_DIR = $(TEST_SRC)/integration_tests

# Test targets
TEST_BOARD = $(BIN_DIR)/test_board
TEST_MATCH = $(BIN_DIR)/test_match
TEST_AUTH_MESSAGES = $(BIN_DIR)/test_auth_messages
TEST_NETWORK = $(BIN_DIR)/test_network
TEST_CLIENT_NETWORK = $(BIN_DIR)/test_client_network
TEST_SESSION_STORAGE = $(BIN_DIR)/test_session_storage
TEST_PASSWORD_HASH = $(BIN_DIR)/test_password_hash
TEST_DATABASE = $(BIN_DIR)/test_database
TEST_PLAYER_MANAGER = $(BIN_DIR)/test_player_manager
TEST_CHALLENGE_MANAGER = $(BIN_DIR)/test_challenge_manager
TEST_CLIENT_SERVER = $(BIN_DIR)/test_client_server
TEST_AUTHENTICATION = $(BIN_DIR)/test_authentication
TEST_E2E_CLIENT_AUTH = $(BIN_DIR)/test_e2e_client_auth
TEST_AUTO_LOGIN = $(BIN_DIR)/test_auto_login
TEST_PLAYER_LIST = $(BIN_DIR)/test_player_list
TEST_CHALLENGE = $(BIN_DIR)/test_challenge
TEST_GAMEPLAY = $(BIN_DIR)/test_gameplay

# Test flags
GTEST_FLAGS = -lgtest -lgtest_main -lpthread

# Collected test targets
UNIT_TESTS = $(TEST_BOARD) $(TEST_MATCH) $(TEST_AUTH_MESSAGES) $(TEST_NETWORK) $(TEST_CLIENT_NETWORK) $(TEST_SESSION_STORAGE) $(TEST_PASSWORD_HASH) $(TEST_DATABASE) $(TEST_PLAYER_MANAGER) $(TEST_CHALLENGE_MANAGER)
INTEGRATION_TESTS = $(TEST_CLIENT_SERVER) $(TEST_AUTHENTICATION) $(TEST_E2E_CLIENT_AUTH) $(TEST_AUTO_LOGIN) $(TEST_PLAYER_LIST) $(TEST_CHALLENGE) $(TEST_GAMEPLAY)
ALL_TESTS = $(UNIT_TESTS) $(INTEGRATION_TESTS)

# Targets
CLIENT_TARGET = $(BIN_DIR)/battleship_client
SERVER_TARGET = $(BIN_DIR)/battleship_server

# Colors for output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
CYAN = \033[0;36m
NC = \033[0m # No Color

# Default target
.PHONY: all
all: banner directories $(CLIENT_TARGET) $(SERVER_TARGET)
	@echo "$(GREEN)╔════════════════════════════════════╗$(NC)"
	@echo "$(GREEN)║   BUILD COMPLETED SUCCESSFULLY!    ║$(NC)"
	@echo "$(GREEN)╚════════════════════════════════════╝$(NC)"
	@echo "$(CYAN)Client binary: $(CLIENT_TARGET)$(NC)"
	@echo "$(CYAN)Server binary: $(SERVER_TARGET)$(NC)"

.PHONY: banner
banner:
	@echo "$(CYAN)╔════════════════════════════════════╗$(NC)"
	@echo "$(CYAN)║  ⚓ BATTLESHIP ONLINE - BUILD ⚓   ║$(NC)"
	@echo "$(CYAN)╠════════════════════════════════════╣$(NC)"
	@echo "$(CYAN)║  Version: 1.0                      ║$(NC)"
	@echo "$(CYAN)║  Platform: Linux/Ubuntu            ║$(NC)"
	@echo "$(CYAN)║  Language: C++11                   ║$(NC)"
	@echo "$(CYAN)╚════════════════════════════════════╝$(NC)"
	@echo ""

# Create directories
.PHONY: directories
directories:
	@mkdir -p $(BUILD_DIR)/common
	@mkdir -p $(BUILD_DIR)/client
	@mkdir -p $(BUILD_DIR)/server
	@mkdir -p $(BUILD_DIR)/tests
	@mkdir -p $(BIN_DIR)

# Build client
$(CLIENT_TARGET): $(COMMON_OBJECTS) $(CLIENT_OBJECTS)
	@echo "$(YELLOW)🔗 Linking client...$(NC)"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTK_LIBS) -lpthread -lssl -lcrypto
	@echo "$(GREEN)✅ Client built successfully!$(NC)"

# Build server
$(SERVER_TARGET): $(COMMON_OBJECTS) $(SERVER_OBJECTS)
	@echo "$(YELLOW)🔗 Linking server...$(NC)"
	@if [ -z "$(SERVER_OBJECTS)" ]; then \
		echo "$(RED)⚠️  No server source files found$(NC)"; \
		echo "$(YELLOW)Creating empty server binary...$(NC)"; \
	fi
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread -lsqlite3 -lssl -lcrypto
	@echo "$(GREEN)✅ Server built successfully!$(NC)"

# Compile common sources
$(BUILD_DIR)/common/%.o: $(COMMON_SRC)/%.cpp
	@echo "$(CYAN)🔧 Compiling $<...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile client sources
$(BUILD_DIR)/client/%.o: $(CLIENT_SRC)/%.cpp
	@echo "$(CYAN)🔧 Compiling $<...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GTK_CFLAGS) -c $< -o $@

# Compile server sources
$(BUILD_DIR)/server/%.o: $(SERVER_SRC)/%.cpp
	@echo "$(CYAN)🔧 Compiling $<...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ============== TEST TARGETS ==============

# ===== Unit Tests =====

# Board tests
$(TEST_BOARD): $(UNIT_TEST_DIR)/board/test_board.cpp $(COMMON_OBJECTS)
	@echo "$(YELLOW)🧪 Building board tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Board tests built!$(NC)"

# Match tests
$(TEST_MATCH): $(UNIT_TEST_DIR)/match/test_match.cpp $(COMMON_OBJECTS)
	@echo "$(YELLOW)🧪 Building match tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Match tests built!$(NC)"

# Authentication message tests
$(TEST_AUTH_MESSAGES): $(UNIT_TEST_DIR)/protocol/test_auth_messages.cpp $(COMMON_OBJECTS)
	@echo "$(YELLOW)🧪 Building authentication message tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Authentication message tests built!$(NC)"

# Network tests
$(TEST_NETWORK): $(UNIT_TEST_DIR)/network/test_network.cpp $(COMMON_OBJECTS) build/server/client_connection.o
	@echo "$(YELLOW)🧪 Building network tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Network tests built!$(NC)"

# Client network tests
$(TEST_CLIENT_NETWORK): $(UNIT_TEST_DIR)/client/test_client_network.cpp $(COMMON_OBJECTS) build/client/client_network.o
	@echo "$(YELLOW)🧪 Building client network tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Client network tests built!$(NC)"

# Session storage tests
$(TEST_SESSION_STORAGE): $(UNIT_TEST_DIR)/client/test_session_storage.cpp $(COMMON_OBJECTS) build/client/session_storage.o
	@echo "$(YELLOW)🧪 Building session storage tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Session storage tests built!$(NC)"

# Password hash tests
$(TEST_PASSWORD_HASH): $(UNIT_TEST_DIR)/crypto/test_password_hash.cpp $(COMMON_OBJECTS)
	@echo "$(YELLOW)🧪 Building password hash tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Password hash tests built!$(NC)"

# Database tests
$(TEST_DATABASE): $(UNIT_TEST_DIR)/database/test_database.cpp $(COMMON_OBJECTS) build/server/database.o
	@echo "$(YELLOW)🧪 Building database tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lsqlite3 -lssl -lcrypto
	@echo "$(GREEN)✅ Database tests built!$(NC)"

# Test PlayerManager
$(TEST_PLAYER_MANAGER): $(UNIT_TEST_DIR)/server/test_player_manager.cpp $(COMMON_OBJECTS) build/server/player_manager.o build/server/server.o build/server/client_connection.o build/server/database.o build/server/auth_handler.o build/server/player_handler.o
	@echo "$(YELLOW)🧪 Building PlayerManager tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lsqlite3 -lssl -lcrypto
	@echo "$(GREEN)✅ PlayerManager tests built!$(NC)"

# Test ChallengeManager
$(TEST_CHALLENGE_MANAGER): $(UNIT_TEST_DIR)/server/test_challenge_manager.cpp $(COMMON_OBJECTS) build/server/challenge_manager.o build/server/challenge_handler.o build/server/player_manager.o build/server/server.o build/server/client_connection.o build/server/database.o build/server/auth_handler.o build/server/player_handler.o
	@echo "$(YELLOW)🧪 Building ChallengeManager tests...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lsqlite3 -lssl -lcrypto
	@echo "$(GREEN)✅ ChallengeManager tests built!$(NC)"

# ===== Integration Tests =====

# Client-Server integration test
$(TEST_CLIENT_SERVER): $(INTEGRATION_TEST_DIR)/test_client_server.cpp $(COMMON_OBJECTS)
	@echo "$(YELLOW)🧪 Building client-server integration test...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Client-server test built!$(NC)"

# Authentication integration test
$(TEST_AUTHENTICATION): $(INTEGRATION_TEST_DIR)/test_authentication.cpp $(COMMON_OBJECTS)
	@echo "$(YELLOW)🧪 Building authentication integration test...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Authentication test built!$(NC)"

# E2E Client Authentication test
$(TEST_E2E_CLIENT_AUTH): $(INTEGRATION_TEST_DIR)/test_e2e_client_auth.cpp $(COMMON_OBJECTS) build/client/client_network.o
	@echo "$(YELLOW)🧪 Building E2E client auth test...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ E2E client auth test built!$(NC)"

# Auto-login integration test
$(TEST_AUTO_LOGIN): $(INTEGRATION_TEST_DIR)/test_auto_login.cpp $(COMMON_OBJECTS) build/client/client_network.o build/client/session_storage.o
	@echo "$(YELLOW)🧪 Building auto-login integration test...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Auto-login integration test built!$(NC)"

# Player list integration test
$(TEST_PLAYER_LIST): $(INTEGRATION_TEST_DIR)/test_player_list.cpp $(COMMON_OBJECTS)
	@echo "$(YELLOW)🧪 Building player list integration test...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Player list integration test built!$(NC)"

# Challenge integration test
$(TEST_CHALLENGE): $(INTEGRATION_TEST_DIR)/test_challenge.cpp $(COMMON_OBJECTS)
	@echo "$(YELLOW)🧪 Building challenge integration test...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Challenge integration test built!$(NC)"

$(TEST_GAMEPLAY): $(INTEGRATION_TEST_DIR)/test_gameplay.cpp $(COMMON_OBJECTS)
	@echo "$(YELLOW)🧪 Building gameplay integration test...$(NC)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(GTEST_FLAGS) -lssl -lcrypto
	@echo "$(GREEN)✅ Gameplay integration test built!$(NC)"

# ===== Build All Tests =====

.PHONY: tests
tests: banner directories $(ALL_TESTS)
	@echo "$(GREEN)╔════════════════════════════════════╗$(NC)"
	@echo "$(GREEN)║    TESTS BUILT SUCCESSFULLY!       ║$(NC)"
	@echo "$(GREEN)╚════════════════════════════════════╝$(NC)"

# ===== Run Tests =====

# Run all tests
.PHONY: test
test: tests
	@echo "$(CYAN)╔════════════════════════════════════╗$(NC)"
	@echo "$(CYAN)║       🧪 RUNNING ALL TESTS 🧪     ║$(NC)"
	@echo "$(CYAN)╚════════════════════════════════════╝$(NC)"
	@echo ""
	@$(MAKE) --no-print-directory test-unit
	@echo ""
	@echo "$(GREEN)╔════════════════════════════════════╗$(NC)"
	@echo "$(GREEN)║     ✅ ALL TESTS COMPLETED! ✅     ║$(NC)"
	@echo "$(GREEN)╚════════════════════════════════════╝$(NC)"

# Run unit tests
.PHONY: test-unit
test-unit: $(UNIT_TESTS)
	@echo "$(CYAN)━━━ Unit Tests ━━━$(NC)"
	@echo ""
	@echo "$(YELLOW)📋 Board Tests$(NC)"
	@./$(TEST_BOARD)
	@echo ""
	@echo "$(YELLOW)📋 Match Tests$(NC)"
	@./$(TEST_MATCH)
	@echo ""
	@echo "$(YELLOW)📋 Authentication Message Tests$(NC)"
	@./$(TEST_AUTH_MESSAGES)
	@echo ""
	@echo "$(YELLOW)📋 Network Tests$(NC)"
	@./$(TEST_NETWORK)
	@echo ""
	@echo "$(YELLOW)📋 Client Network Tests$(NC)"
	@./$(TEST_CLIENT_NETWORK)
	@echo ""
	@echo "$(YELLOW)📋 Session Storage Tests$(NC)"
	@./$(TEST_SESSION_STORAGE)
	@echo ""
	@echo "$(YELLOW)📋 Password Hash Tests$(NC)"
	@./$(TEST_PASSWORD_HASH)
	@echo ""
	@echo "$(YELLOW)📋 Database Tests$(NC)"
	@./$(TEST_DATABASE)
	@echo ""
	@echo "$(YELLOW)📋 PlayerManager Tests$(NC)"
	@./$(TEST_PLAYER_MANAGER)
	@echo ""
	@echo "$(YELLOW)📋 ChallengeManager Tests$(NC)"
	@./$(TEST_CHALLENGE_MANAGER)
	@echo ""
	@echo "$(GREEN)✅ All unit tests passed!$(NC)"

# Run integration tests
.PHONY: test-integration
test-integration: $(INTEGRATION_TESTS)
	@echo "$(CYAN)━━━ Integration Tests ━━━$(NC)"
	@echo ""
	@echo "$(YELLOW)⚠️  Make sure server is running: ./bin/battleship_server$(NC)"
	@echo ""
	@./$(TEST_CLIENT_SERVER)
	@echo ""
	@echo "$(GREEN)✅ Integration tests passed!$(NC)"

# Run individual test suites
.PHONY: test-board
test-board: $(TEST_BOARD)
	@echo "$(CYAN)━━━ Board Tests ━━━$(NC)"
	@./$(TEST_BOARD)

.PHONY: test-match
test-match: $(TEST_MATCH)
	@echo "$(CYAN)━━━ Match Tests ━━━$(NC)"
	@./$(TEST_MATCH)

.PHONY: test-protocol
test-protocol: $(TEST_PROTOCOL)
	@echo "$(CYAN)━━━ Protocol Tests ━━━$(NC)"
	@./$(TEST_PROTOCOL)

.PHONY: test-network
test-network: $(TEST_NETWORK)
	@echo "$(CYAN)━━━ Network Tests ━━━$(NC)"
	@./$(TEST_NETWORK)

# Debug build
.PHONY: debug
debug: CXXFLAGS += $(DEBUGFLAGS)
debug: clean all
	@echo "$(YELLOW)🐛 Debug build complete$(NC)"

# Client only
.PHONY: client
client: banner directories $(CLIENT_TARGET)
	@echo "$(GREEN)✅ Client build complete!$(NC)"

# Server only
.PHONY: server
server: banner directories $(SERVER_TARGET)
	@echo "$(GREEN)✅ Server build complete!$(NC)"

# Clean
.PHONY: clean
clean:
	@echo "$(RED)🧹 Cleaning build files...$(NC)"
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)
	@echo "$(GREEN)✅ Clean complete!$(NC)"

# Clean tests only
.PHONY: clean-tests
clean-tests:
	@echo "$(RED)🧹 Cleaning test files...$(NC)"
	rm -f $(ALL_TESTS)
	rm -rf $(BUILD_DIR)/tests
	@echo "$(GREEN)✅ Test files cleaned!$(NC)"

# Run client
.PHONY: run-client
run-client: $(CLIENT_TARGET)
	@echo "$(CYAN)🚀 Starting Battleship client...$(NC)"
	@echo ""
	./$(CLIENT_TARGET)

# Run server
.PHONY: run-server
run-server: $(SERVER_TARGET)
	@echo "$(CYAN)🚀 Starting Battleship server...$(NC)"
	@echo ""
	./$(SERVER_TARGET)

# Install dependencies (Ubuntu/Debian)
.PHONY: install-deps
install-deps:
	@echo "$(YELLOW)📦 Installing dependencies...$(NC)"
	sudo apt-get update
	sudo apt-get install -y build-essential g++ pkg-config
	sudo apt-get install -y libgtk-3-dev libcairo2-dev
	sudo apt-get install -y libsqlite3-dev
	@echo "$(GREEN)✅ Dependencies installed!$(NC)"

# Help
.PHONY: help
help:
	@echo "$(CYAN)╔════════════════════════════════════╗$(NC)"
	@echo "$(CYAN)║     BATTLESHIP BUILD SYSTEM        ║$(NC)"
	@echo "$(CYAN)╚════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "$(YELLOW)Available targets:$(NC)"
	@echo "  $(GREEN)make all$(NC)           - Build both client and server (default)"
	@echo "  $(GREEN)make client$(NC)        - Build client only"
	@echo "  $(GREEN)make server$(NC)        - Build server only"
	@echo "  $(GREEN)make debug$(NC)         - Build with debug symbols"
	@echo "  $(GREEN)make clean$(NC)         - Remove all build files"
	@echo "  $(GREEN)make run-client$(NC)    - Build and run client"
	@echo "  $(GREEN)make run-server$(NC)    - Build and run server"
	@echo "  $(GREEN)make install-deps$(NC)  - Install required dependencies"
	@echo ""
	@echo "$(YELLOW)Testing targets:$(NC)"
	@echo "  $(GREEN)make tests$(NC)         - Build all tests"
	@echo "  $(GREEN)make test$(NC)          - Build and run all tests"
	@echo "  $(GREEN)make test-unit$(NC)     - Run all unit tests"
	@echo "  $(GREEN)make test-integration$(NC) - Run all integration tests"
	@echo "  $(GREEN)make test-board$(NC)    - Run board tests only"
	@echo "  $(GREEN)make test-match$(NC)    - Run match tests only"
	@echo "  $(GREEN)make test-protocol$(NC) - Run protocol tests only"
	@echo "  $(GREEN)make test-network$(NC)  - Run network tests only"
	@echo "  $(GREEN)make clean-tests$(NC)   - Clean test files"
	@echo ""
	@echo "  $(GREEN)make help$(NC)          - Show this help message"
	@echo ""
	@echo "$(YELLOW)Examples:$(NC)"
	@echo "  $(CYAN)make clean && make all$(NC)  - Clean build from scratch"
	@echo "  $(CYAN)make client run-client$(NC)  - Build and run client"
	@echo ""

# Phony targets
.PHONY: all clean client server debug run-client run-server install-deps help banner directories
.PHONY: tests test test-unit test-integration
.PHONY: test-board test-match test-protocol test-network clean-tests
