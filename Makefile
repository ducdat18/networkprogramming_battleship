# Makefile for Battleship Online Game
# Platform: Linux/Ubuntu
# Language: C++

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
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
BUILD_DIR = build
BIN_DIR = bin

# Include paths
INCLUDES = -I$(CLIENT_INC) -I$(SERVER_INC) -I$(COMMON_INC)

# Source files
COMMON_SOURCES = $(wildcard $(COMMON_SRC)/*.cpp)
CLIENT_SOURCES = $(wildcard $(CLIENT_SRC)/*.cpp)
SERVER_SOURCES = $(wildcard $(SERVER_SRC)/*.cpp)

# Object files
COMMON_OBJECTS = $(patsubst $(COMMON_SRC)/%.cpp,$(BUILD_DIR)/common/%.o,$(COMMON_SOURCES))
CLIENT_OBJECTS = $(patsubst $(CLIENT_SRC)/%.cpp,$(BUILD_DIR)/client/%.o,$(CLIENT_SOURCES))
SERVER_OBJECTS = $(patsubst $(SERVER_SRC)/%.cpp,$(BUILD_DIR)/server/%.o,$(SERVER_SOURCES))

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
	@mkdir -p $(BIN_DIR)

# Build client
$(CLIENT_TARGET): $(COMMON_OBJECTS) $(CLIENT_OBJECTS)
	@echo "$(YELLOW)🔗 Linking client...$(NC)"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTK_LIBS) -lpthread
	@echo "$(GREEN)✅ Client built successfully!$(NC)"

# Build server
$(SERVER_TARGET): $(COMMON_OBJECTS) $(SERVER_OBJECTS)
	@echo "$(YELLOW)🔗 Linking server...$(NC)"
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread -lsqlite3
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
	@echo "  $(GREEN)make help$(NC)          - Show this help message"
	@echo ""
	@echo "$(YELLOW)Examples:$(NC)"
	@echo "  $(CYAN)make clean && make all$(NC)  - Clean build from scratch"
	@echo "  $(CYAN)make client run-client$(NC)  - Build and run client"
	@echo ""

# Phony targets
.PHONY: all clean client server debug run-client run-server install-deps help banner directories
