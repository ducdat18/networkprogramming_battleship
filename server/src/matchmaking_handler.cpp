#include "matchmaking_handler.h"
#include "server.h"
#include "database.h"
#include <cstring>
#include <iostream>

MatchmakingHandler::MatchmakingHandler(Server* server, MatchmakingManager* mgr, DatabaseManager* db)
    : server_(server), matchmaking_mgr_(mgr), db_(db) {
}

MatchmakingHandler::~MatchmakingHandler() {
}

bool MatchmakingHandler::canHandle(MessageType type) const {
    return (type == MessageType::QUEUE_JOIN ||
            type == MessageType::QUEUE_LEAVE);
}

bool MatchmakingHandler::handleMessage(ClientConnection* client,
                                       const MessageHeader& header,
                                       const std::string& payload) {
    if (!client) return false;

    int client_fd = client->getSocketFd();
    MessageType type = static_cast<MessageType>(header.type);

    switch (type) {
        case MessageType::QUEUE_JOIN: {
            QueueJoinMessage msg;
            if (payload.size() >= sizeof(QueueJoinMessage)) {
                memcpy(&msg, payload.data(), sizeof(QueueJoinMessage));
            } else {
                // Use default values if no payload
                msg.time_limit = 60;
            }
            handleQueueJoin(header, msg, client_fd);
            return true;
        }

        case MessageType::QUEUE_LEAVE: {
            QueueLeaveMessage msg;
            if (payload.size() >= sizeof(QueueLeaveMessage)) {
                memcpy(&msg, payload.data(), sizeof(QueueLeaveMessage));
            }
            // Empty message is fine for leave queue
            handleQueueLeave(header, msg, client_fd);
            return true;
        }

        default:
            break;
    }

    return false;
}

void MatchmakingHandler::handleQueueJoin(const MessageHeader& header,
                                        const QueueJoinMessage& msg,
                                        int client_fd) {
    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) {
        std::cout << "[MATCHMAKING] Invalid session token" << std::endl;
        return;
    }

    // Join queue
    bool success = matchmaking_mgr_->joinQueue(user_id, msg.time_limit);

    // Send status back to client
    QueueStatusMessage status = matchmaking_mgr_->getQueueStatus(user_id);

    MessageHeader resp_header;
    memset(&resp_header, 0, sizeof(resp_header));
    resp_header.type = MessageType::QUEUE_STATUS;
    resp_header.length = sizeof(status);
    resp_header.timestamp = time(nullptr);

    server_->sendToClient(client_fd, resp_header, &status, sizeof(status));

    if (success) {
        std::cout << "[MATCHMAKING] User " << user_id << " joined queue successfully" << std::endl;
    }
}

void MatchmakingHandler::handleQueueLeave(const MessageHeader& header,
                                         const QueueLeaveMessage& msg,
                                         int client_fd) {
    (void)msg; // Unused

    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) {
        return;
    }

    // Leave queue
    bool success = matchmaking_mgr_->leaveQueue(user_id);

    // Send status back to client (in_queue = false)
    QueueStatusMessage status;
    status.in_queue = false;

    MessageHeader resp_header;
    memset(&resp_header, 0, sizeof(resp_header));
    resp_header.type = MessageType::QUEUE_STATUS;
    resp_header.length = sizeof(status);
    resp_header.timestamp = time(nullptr);

    server_->sendToClient(client_fd, resp_header, &status, sizeof(status));

    if (success) {
        std::cout << "[MATCHMAKING] User " << user_id << " left queue successfully" << std::endl;
    }
}
