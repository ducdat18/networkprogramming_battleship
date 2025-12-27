#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <map>
#include <mutex>
#include <vector>
#include "protocol.h"
#include "messages/matchmaking_messages.h"

// Forward declaration
class ClientConnection;
class Server;

/**
 * PlayerManager - Manages online players and their status
 *
 * Responsibilities:
 * - Track online players and their status
 * - Provide player list to clients
 * - Broadcast player status updates
 */
class PlayerManager {
public:
    PlayerManager(Server* server);
    ~PlayerManager();

    // Player registration (when user logs in)
    void addPlayer(ClientConnection* client, uint32_t user_id,
                   const std::string& username, const std::string& display_name,
                   int32_t elo_rating);

    // Player removal (when user disconnects/logs out)
    void removePlayer(uint32_t user_id);

    // Update player status
    void updatePlayerStatus(uint32_t user_id, PlayerStatus status);

    // Get player status
    PlayerStatus getPlayerStatus(uint32_t user_id) const;

    // Get list of online players
    std::vector<PlayerInfo_Message> getOnlinePlayers() const;

    // Get list of available players (for challenging)
    std::vector<PlayerInfo_Message> getAvailablePlayers() const;

    // Get player info by user_id
    PlayerInfo_Message getPlayerInfo(uint32_t user_id) const;

    // Get client connection by user_id
    ClientConnection* getClientConnection(uint32_t user_id) const;

    // Check if player is online
    bool isPlayerOnline(uint32_t user_id) const;

    // Broadcast status update to all clients
    void broadcastPlayerStatusUpdate(uint32_t user_id, PlayerStatus status);

private:
    struct PlayerData {
        uint32_t user_id;
        std::string username;
        std::string display_name;
        int32_t elo_rating;
        PlayerStatus status;
        ClientConnection* client;

        PlayerData() : user_id(0), elo_rating(1000),
                      status(STATUS_OFFLINE), client(nullptr) {}
    };

    Server* server_;
    mutable std::mutex mutex_;
    std::map<uint32_t, PlayerData> players_; // user_id -> PlayerData
};

#endif // PLAYER_MANAGER_H
