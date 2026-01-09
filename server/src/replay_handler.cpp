#include "replay_handler.h"
#include "server.h"
#include "messages/replay_messages.h"
#include <cstring>
#include <iostream>

ReplayHandler::ReplayHandler(Server* server, DatabaseManager* db)
    : server_(server), db_(db) {
    std::cout << "[REPLAY_HANDLER] Initialized" << std::endl;
}

ReplayHandler::~ReplayHandler() {
}

bool ReplayHandler::canHandle(MessageType type) const {
    return (type == MessageType::REPLAY_REQUEST);
}

bool ReplayHandler::handleMessage(ClientConnection* client,
                                  const MessageHeader& header,
                                  const std::string& payload) {
    if (!client) return false;

    int client_fd = client->getSocketFd();
    MessageType type = static_cast<MessageType>(header.type);

    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) {
        std::cout << "[REPLAY_HANDLER] Invalid session token" << std::endl;
        return false;
    }

    switch (type) {
        case MessageType::REPLAY_REQUEST: {
            ReplayListRequest req;
            if (payload.size() >= sizeof(ReplayListRequest)) {
                memcpy(&req, payload.data(), sizeof(ReplayListRequest));
            } else {
                // Use default values if no payload
                req.user_id = user_id;  // Get current user's matches
                req.limit = 20;
                req.offset = 0;
            }
            
            // If req.user_id is 0, use the authenticated user's ID
            if (req.user_id == 0) {
                req.user_id = user_id;
            }
            
            handleReplayListRequest(header, client_fd, req.user_id);
            return true;
        }

        default:
            break;
    }

    return false;
}

void ReplayHandler::handleReplayListRequest(const MessageHeader& header,
                                           int client_fd,
                                           uint32_t user_id) {
    std::cout << "[REPLAY_HANDLER] Replay list requested for user " << user_id << std::endl;

    // Get match history from database
    std::vector<Match> matches = db_->getMatchHistory(user_id, 50, 0);

    ReplayListResponse resp;
    resp.total_count = matches.size();
    resp.count = std::min((uint32_t)matches.size(), 50u);

    // Convert Match to ReplayMatchInfo
    for (size_t i = 0; i < resp.count && i < matches.size(); i++) {
        const Match& match = matches[i];
        ReplayMatchInfo& info = resp.matches[i];

        info.match_id = match.match_id;
        info.player1_id = match.player1_id;
        info.player2_id = match.player2_id;
        info.winner_id = match.winner_id;
        
        // Get player names from database
        User p1 = db_->getUserById(match.player1_id);
        User p2 = db_->getUserById(match.player2_id);
        
        strncpy(info.player1_name, p1.display_name.c_str(), sizeof(info.player1_name) - 1);
        strncpy(info.player2_name, p2.display_name.c_str(), sizeof(info.player2_name) - 1);
        info.player1_name[sizeof(info.player1_name) - 1] = '\0';
        info.player2_name[sizeof(info.player2_name) - 1] = '\0';
        
        // TODO: Get ELO changes from replay metadata
        info.player1_elo_before = p1.elo_rating;
        info.player2_elo_before = p2.elo_rating;
        info.player1_elo_after = p1.elo_rating;
        info.player2_elo_after = p2.elo_rating;
        
        info.created_at = match.created_at;
        info.ended_at = match.ended_at;
        info.duration_seconds = (match.ended_at > match.created_at) ? 
                                (match.ended_at - match.created_at) : 0;
        
        // Get move count from database
        std::vector<std::string> moves = db_->getMatchMoves(match.match_id);
        info.total_moves = moves.size();
        
        strncpy(info.end_reason, match.status.c_str(), sizeof(info.end_reason) - 1);
        info.end_reason[sizeof(info.end_reason) - 1] = '\0';
    }

    std::cout << "[REPLAY_HANDLER] Sending " << resp.count << " matches to client" << std::endl;

    // Send response
    MessageHeader resp_header;
    memset(&resp_header, 0, sizeof(resp_header));
    resp_header.type = MessageType::REPLAY_DATA;
    resp_header.length = sizeof(resp);
    resp_header.timestamp = time(nullptr);

    server_->sendToClient(client_fd, resp_header, &resp, sizeof(resp));
}
