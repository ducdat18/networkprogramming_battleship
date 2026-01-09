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
class DatabaseManager;
class PlayerManager;
class ChallengeManager;
class GameplayHandler;
class MatchmakingManager;
class MatchmakingHandler;

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

    // Managers
    PlayerManager* getPlayerManager() { return player_manager_; }
    ChallengeManager* getChallengeManager() { return challenge_manager_; }
    GameplayHandler* getGameplayHandler() { return gameplay_handler_; }
    MatchmakingManager* getMatchmakingManager() { return matchmaking_mgr_; }
    DatabaseManager* getDatabase() { return db_; }
    DatabaseManager* getDatabaseManager() { return db_; }

    // Broadcasting and messaging
    void broadcast(const MessageHeader& header, const std::string& payload);
    bool sendToClient(int client_fd, const MessageHeader& header, const void* payload, size_t payload_size);

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

    // Database
    DatabaseManager* db_;

    // Managers
    PlayerManager* player_manager_;
    ChallengeManager* challenge_manager_;
    GameplayHandler* gameplay_handler_;
    MatchmakingManager* matchmaking_mgr_;
    MatchmakingHandler* matchmaking_handler_;

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
