#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <atomic>
#include "protocol.h"

// Forward declarations
class ClientConnection;
class MessageHandler;

/**
 * Main server class for Battleship game
 * Handles socket operations, client connections, and routing
 */
class Server {
public:
    Server(int port = DEFAULT_PORT);
    ~Server();

    // Server lifecycle
    bool start();
    void stop();
    bool isRunning() const { return running_; }

    // Statistics
    int getConnectedClients() const;
    int getActiveMatches() const;

private:
    // Socket operations
    bool createSocket();
    bool bindSocket();
    bool listenSocket();
    void acceptConnections(); // Main accept loop

    // Client management
    void handleClient(int client_fd);
    void removeClient(int client_fd);
    void broadcastToAll(const std::string& message);

    // Message routing
    void setupHandlers();
    bool routeMessage(ClientConnection* client,
                     const MessageHeader& header,
                     const std::string& payload);

    // Configuration
    int port_;
    int server_fd_;
    std::atomic<bool> running_;

    // Client tracking
    std::map<int, std::shared_ptr<ClientConnection>> clients_;
    mutable std::mutex clients_mutex_;

    // Statistics
    std::atomic<int> total_connections_;
    std::atomic<int> active_matches_;

    // Message handlers
    std::vector<MessageHandler*> handlers_;
    std::mutex handlers_mutex_;
};

#endif // SERVER_H
