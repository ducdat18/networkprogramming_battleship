#include "player_handler.h"
#include "server.h"
#include "player_manager.h"
#include "client_connection.h"
#include "messages/matchmaking_messages.h"
#include "message_serialization.h"
#include <iostream>
#include <cstring>

using namespace MessageSerialization;

PlayerHandler::PlayerHandler(Server* server, PlayerManager* player_manager)
    : server_(server), player_manager_(player_manager) {
    std::cout << "[PLAYER_HANDLER] Initialized" << std::endl;
}

bool PlayerHandler::canHandle(MessageType type) const {
    return type == MessageType::PLAYER_LIST_REQUEST;
}

bool PlayerHandler::handleMessage(ClientConnection* client,
                                  const MessageHeader& header,
                                  const std::string& payload) {
    MessageType type = static_cast<MessageType>(header.type);

    switch (type) {
        case MessageType::PLAYER_LIST_REQUEST:
            return handlePlayerListRequest(client, payload);
        default:
            return false;
    }
}

bool PlayerHandler::handlePlayerListRequest(ClientConnection* client, const std::string& payload) {
    (void)payload;
    std::cout << "[PLAYER_HANDLER] Player list request from client" << std::endl;

    // Get list of online players
    std::vector<PlayerInfo_Message> online_players = player_manager_->getOnlinePlayers();

    // Debug: Print all online players
    std::cout << "[PLAYER_HANDLER] Total online players in manager: " << online_players.size() << std::endl;
    for (const auto& player : online_players) {
        std::cout << "[PLAYER_HANDLER]   - Player: " << player.display_name 
                  << " (ID: " << player.user_id 
                  << ", Status: " << (int)player.status << ")" << std::endl;
    }

    // Create response
    PlayerListResponse response;
    response.count = std::min(static_cast<size_t>(100), online_players.size());

    // Copy players to response (max 100)
    for (size_t i = 0; i < response.count; i++) {
        response.players[i] = online_players[i];
    }

    std::cout << "[PLAYER_HANDLER] Sending " << response.count << " players to client" << std::endl;

    // Send response
    MessageHeader response_header;
    response_header.type = static_cast<uint8_t>(MessageType::PLAYER_LIST);
    response_header.length = sizeof(PlayerListResponse);
    response_header.timestamp = time(nullptr);
    memset(response_header.session_token, 0, sizeof(response_header.session_token));

    return client->sendMessage(response_header, serialize(response));
}
