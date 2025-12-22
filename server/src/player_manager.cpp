#include "player_manager.h"
#include "server.h"
#include "client_connection.h"
#include "message_serialization.h"
#include <iostream>
#include <cstring>

using namespace MessageSerialization;

PlayerManager::PlayerManager(Server* server) : server_(server) {
    std::cout << "[PLAYER_MANAGER] Initialized" << std::endl;
}

PlayerManager::~PlayerManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    players_.clear();
}

void PlayerManager::addPlayer(ClientConnection* client, uint32_t user_id,
                               const std::string& username, const std::string& display_name,
                               int32_t elo_rating) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if player already exists (same user_id from different connection)
        auto it = players_.find(user_id);
        if (it != players_.end()) {
            std::cout << "[PLAYER_MANAGER] WARNING: Player " << display_name
                      << " (ID: " << user_id << ") already exists! Overwriting..." << std::endl;
        }

        PlayerData data;
        data.user_id = user_id;
        data.username = username;
        data.display_name = display_name;
        data.elo_rating = elo_rating;
        data.status = STATUS_AVAILABLE; // New players are available
        data.client = client;

        players_[user_id] = data;

        std::cout << "[PLAYER_MANAGER] Player added: " << display_name
                  << " (ID: " << user_id << ", ELO: " << elo_rating 
                  << "). Total players now: " << players_.size() << std::endl;
    } // Release lock before broadcasting

    // Broadcast status update to all clients (outside lock to avoid deadlock)
    broadcastPlayerStatusUpdate(user_id, STATUS_AVAILABLE);
}

void PlayerManager::removePlayer(uint32_t user_id) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = players_.find(user_id);
        if (it != players_.end()) {
            std::cout << "[PLAYER_MANAGER] Player removed: " << it->second.display_name
                      << " (ID: " << user_id << ")" << std::endl;

            players_.erase(it);
        } else {
            return; // Player not found, nothing to do
        }
    } // Release lock before broadcasting

    // Broadcast offline status after removing (outside lock to avoid deadlock)
    broadcastPlayerStatusUpdate(user_id, STATUS_OFFLINE);
}

void PlayerManager::updatePlayerStatus(uint32_t user_id, PlayerStatus status) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = players_.find(user_id);
        if (it != players_.end()) {
            it->second.status = status;

            std::cout << "[PLAYER_MANAGER] Player status updated: " << it->second.display_name
                      << " -> " << static_cast<int>(status) << std::endl;
        } else {
            return; // Player not found, nothing to do
        }
    } // Release lock before broadcasting

    // Broadcast status update (outside lock to avoid deadlock)
    broadcastPlayerStatusUpdate(user_id, status);
}

PlayerStatus PlayerManager::getPlayerStatus(uint32_t user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = players_.find(user_id);
    if (it != players_.end()) {
        return it->second.status;
    }
    return STATUS_OFFLINE;
}

std::vector<PlayerInfo_Message> PlayerManager::getOnlinePlayers() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PlayerInfo_Message> result;
    for (const auto& pair : players_) {
        const PlayerData& data = pair.second;

        PlayerInfo_Message info;
        info.user_id = data.user_id;
        safeStrCopy(info.username, data.username, sizeof(info.username));
        safeStrCopy(info.display_name, data.display_name, sizeof(info.display_name));
        info.elo_rating = data.elo_rating;
        info.status = data.status;

        result.push_back(info);
    }

    return result;
}

std::vector<PlayerInfo_Message> PlayerManager::getAvailablePlayers() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PlayerInfo_Message> result;
    for (const auto& pair : players_) {
        const PlayerData& data = pair.second;

        // Only include available players
        if (data.status == STATUS_AVAILABLE) {
            PlayerInfo_Message info;
            info.user_id = data.user_id;
            safeStrCopy(info.username, data.username, sizeof(info.username));
            safeStrCopy(info.display_name, data.display_name, sizeof(info.display_name));
            info.elo_rating = data.elo_rating;
            info.status = data.status;

            result.push_back(info);
        }
    }

    return result;
}

PlayerInfo_Message PlayerManager::getPlayerInfo(uint32_t user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    PlayerInfo_Message info;
    auto it = players_.find(user_id);
    if (it != players_.end()) {
        const PlayerData& data = it->second;
        info.user_id = data.user_id;
        safeStrCopy(info.username, data.username, sizeof(info.username));
        safeStrCopy(info.display_name, data.display_name, sizeof(info.display_name));
        info.elo_rating = data.elo_rating;
        info.status = data.status;
    }

    return info;
}

ClientConnection* PlayerManager::getClientConnection(uint32_t user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = players_.find(user_id);
    if (it != players_.end()) {
        return it->second.client;
    }
    return nullptr;
}

bool PlayerManager::isPlayerOnline(uint32_t user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return players_.find(user_id) != players_.end();
}

void PlayerManager::broadcastPlayerStatusUpdate(uint32_t user_id, PlayerStatus status) {
    // Get player info (must be outside the lock to avoid deadlock)
    PlayerInfo_Message info;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = players_.find(user_id);
        if (it != players_.end()) {
            const PlayerData& data = it->second;
            info.user_id = data.user_id;
            safeStrCopy(info.username, data.username, sizeof(info.username));
            safeStrCopy(info.display_name, data.display_name, sizeof(info.display_name));
            info.elo_rating = data.elo_rating;
            info.status = status;
        } else {
            // Player not found, create offline status
            info.user_id = user_id;
            info.status = STATUS_OFFLINE;
        }
    }

    // Create status update message
    PlayerStatusUpdate update;
    update.user_id = info.user_id;
    update.status = info.status;
    safeStrCopy(update.display_name, info.display_name, sizeof(update.display_name));
    update.elo_rating = info.elo_rating;

    // Broadcast to all connected clients (only if server is running)
    if (server_ && server_->isRunning()) {
        MessageHeader header;
        header.type = static_cast<uint8_t>(MessageType::PLAYER_STATUS_UPDATE);
        header.length = sizeof(PlayerStatusUpdate);
        header.timestamp = time(nullptr);
        memset(header.session_token, 0, sizeof(header.session_token));

        std::cout << "[PLAYER_MANAGER] Broadcasting status update: "
                  << info.display_name << " (ID: " << user_id 
                  << ") -> Status: " << static_cast<int>(status) << std::endl;
        
        server_->broadcast(header, serialize(update));

        std::cout << "[PLAYER_MANAGER] Broadcast sent to all clients" << std::endl;
    } else {
        std::cerr << "[PLAYER_MANAGER] WARNING: Cannot broadcast - server not running!" << std::endl;
    }
}
