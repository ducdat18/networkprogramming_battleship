#include "server.h"
#include "client_connection.h"
#include "message_handler.h"
#include "auth_handler.h"
#include "player_handler.h"
#include "challenge_handler.h"
#include "database.h"
#include "player_manager.h"
#include "challenge_manager.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Server::Server(int port)
    : port_(port)
    , server_fd_(-1)
    , running_(false)
    , db_(nullptr)
    , player_manager_(nullptr)
    , challenge_manager_(nullptr)
    , total_connections_(0)
    , active_matches_(0)
{
    // Initialize database
    db_ = new DatabaseManager("data/battleship.db");
    if (!db_->isOpen()) {
        std::cerr << "[SERVER] Failed to open database!" << std::endl;
        delete db_;
        db_ = nullptr;
    } else {
        std::cout << "[SERVER] Database initialized successfully" << std::endl;
    }

    // Initialize player manager
    player_manager_ = new PlayerManager(this);

    // Initialize challenge manager
    challenge_manager_ = new ChallengeManager(this, player_manager_);
}

Server::~Server() {
    stop();

    // Cleanup handlers
    for (auto handler : handlers_) {
        delete handler;
    }
    handlers_.clear();

    // Cleanup challenge manager
    if (challenge_manager_) {
        delete challenge_manager_;
        challenge_manager_ = nullptr;
    }

    // Cleanup player manager
    if (player_manager_) {
        delete player_manager_;
        player_manager_ = nullptr;
    }

    // Cleanup database
    if (db_) {
        delete db_;
        db_ = nullptr;
    }
}

bool Server::start() {
    if (running_) {
        std::cerr << "[SERVER] Already running" << std::endl;
        return false;
    }

    // Setup message handlers
    setupHandlers();

    // Create socket
    if (!createSocket()) {
        return false;
    }

    // Bind to port
    if (!bindSocket()) {
        close(server_fd_);
        return false;
    }

    // Listen for connections
    if (!listenSocket()) {
        close(server_fd_);
        return false;
    }

    running_ = true;

    // Start accept thread
    std::thread accept_thread(&Server::acceptConnections, this);
    accept_thread.detach();

    return true;
}

void Server::setupHandlers() {
    std::lock_guard<std::mutex> lock(handlers_mutex_);

    std::cout << "[SERVER] Setting up message handlers..." << std::endl;

    // Add handlers for different message types
    if (db_ && db_->isOpen()) {
        AuthHandler* auth_handler = new AuthHandler(db_);
        auth_handler->setServer(this); // Set server reference for PlayerManager access
        handlers_.push_back(auth_handler);
    } else {
        std::cerr << "[SERVER] Cannot create AuthHandler: database not available" << std::endl;
    }

    // Add player handler
    if (player_manager_) {
        handlers_.push_back(new PlayerHandler(this, player_manager_));
    }

    // Add challenge handler
    if (challenge_manager_) {
        handlers_.push_back(new ChallengeHandler(this, challenge_manager_));
    }

    // TODO: Add other handlers
    // handlers_.push_back(new ChallengeHandler());
    // handlers_.push_back(new GameplayHandler());

    std::cout << "[SERVER] " << handlers_.size() << " handlers registered" << std::endl;
}

void Server::stop() {
    if (!running_) {
        return;
    }

    std::cout << "[SERVER] Stopping server..." << std::endl;
    running_ = false;

    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& pair : clients_) {
            pair.second->disconnect();
        }
        clients_.clear();
    }

    // Close server socket
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }

    std::cout << "[SERVER] Server stopped" << std::endl;
}

bool Server::createSocket() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[ERROR] Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[WARNING] setsockopt SO_REUSEADDR failed: " << strerror(errno) << std::endl;
    }

    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "[WARNING] setsockopt SO_REUSEPORT failed: " << strerror(errno) << std::endl;
    }

    std::cout << "[SERVER] Socket created (fd=" << server_fd_ << ")" << std::endl;
    return true;
}

bool Server::bindSocket() {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[ERROR] Failed to bind to port " << port_
                  << ": " << strerror(errno) << std::endl;
        return false;
    }

    std::cout << "[SERVER] Bound to port " << port_ << std::endl;
    return true;
}

bool Server::listenSocket() {
    const int BACKLOG = 128;  // Max pending connections

    if (listen(server_fd_, BACKLOG) < 0) {
        std::cerr << "[ERROR] Failed to listen: " << strerror(errno) << std::endl;
        return false;
    }

    std::cout << "[SERVER] Listening for connections (backlog=" << BACKLOG << ")" << std::endl;
    return true;
}

void Server::acceptConnections() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    std::cout << "[SERVER] Accept thread started" << std::endl;

    while (running_) {
        // Accept new connection
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &addr_len);

        if (!running_) {
            break;  // Server stopped
        }

        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted, try again
            }
            std::cerr << "[ERROR] Accept failed: " << strerror(errno) << std::endl;
            continue;
        }

        // Get client info
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        std::cout << "[CONNECTION] New client connected: " << client_ip
                  << ":" << client_port << " (fd=" << client_fd << ")" << std::endl;

        total_connections_++;

        // Create client connection object
        auto client = std::make_shared<ClientConnection>(client_fd);

        // Add to clients map
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            clients_[client_fd] = client;
        }

        // Handle client in separate thread
        std::thread client_thread(&Server::handleClient, this, client_fd);
        client_thread.detach();
    }

    std::cout << "[SERVER] Accept thread stopped" << std::endl;
}

void Server::handleClient(int client_fd) {
    std::cout << "[THREAD] Handler thread started for client fd=" << client_fd << std::endl;

    std::shared_ptr<ClientConnection> client;

    // Get client connection
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        auto it = clients_.find(client_fd);
        if (it == clients_.end()) {
            std::cerr << "[ERROR] Client fd=" << client_fd << " not found!" << std::endl;
            return;
        }
        client = it->second;
    }

    // Main message loop
    while (running_ && client->isConnected()) {
        MessageHeader header;
        std::string payload;

        // Receive message
        if (!client->receiveMessage(header, payload)) {
            std::cout << "[DISCONNECT] Client fd=" << client_fd << " disconnected" << std::endl;
            break;
        }

        std::cout << "[MESSAGE] Received from fd=" << client_fd
                  << " type=" << (int)header.type
                  << " length=" << header.length << std::endl;

        // Route message to appropriate handler
        if (!routeMessage(client.get(), header, payload)) {
            std::cerr << "[ERROR] Failed to route message type=" << (int)header.type << std::endl;
        }
    }

    // Cleanup
    removeClient(client_fd);

    std::cout << "[THREAD] Handler thread stopped for client fd=" << client_fd << std::endl;
}

void Server::removeClient(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_fd);
    if (it != clients_.end()) {
        it->second->disconnect();
        clients_.erase(it);
        std::cout << "[CLEANUP] Removed client fd=" << client_fd << std::endl;
    }
}

void Server::broadcastToAll(const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    for (auto& pair : clients_) {
        // TODO: Implement broadcast
    }
}

void Server::broadcast(const MessageHeader& header, const std::string& payload) {
    // Copy client list to avoid holding lock during I/O
    std::vector<std::shared_ptr<ClientConnection>> clients_copy;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_copy.reserve(clients_.size());
        for (auto& pair : clients_) {
            if (pair.second) {
                clients_copy.push_back(pair.second);
            }
        }
    } // Release lock before sending

    // Send to all clients without holding lock (avoid blocking)
    for (auto& client : clients_copy) {
        client->sendMessage(header, payload);
    }
}

int Server::getConnectedClients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

int Server::getActiveMatches() const {
    return active_matches_;
}

bool Server::routeMessage(ClientConnection* client,
                         const MessageHeader& header,
                         const std::string& payload) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);

    // Try PING/PONG first (keep for backwards compatibility)
    if (header.type == static_cast<uint8_t>(PING)) {
        MessageHeader pong_header;
        pong_header.type = static_cast<uint8_t>(PONG);
        pong_header.length = 0;
        pong_header.timestamp = time(nullptr);
        memset(pong_header.session_token, 0, sizeof(pong_header.session_token));

        return client->sendMessage(pong_header, "");
    }

    // Route to registered handlers
    for (auto handler : handlers_) {
        if (handler->canHandle(static_cast<MessageType>(header.type))) {
            return handler->handleMessage(client, header, payload);
        }
    }

    // No handler found
    std::cerr << "[ROUTER] No handler for message type=" << (int)header.type << std::endl;
    return false;
}
