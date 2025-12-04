#include "challenge_handler.h"
#include "challenge_manager.h"
#include "server.h"
#include "client_connection.h"
#include "message_serialization.h"
#include "messages/matchmaking_messages.h"
#include <iostream>

using namespace MessageSerialization;

ChallengeHandler::ChallengeHandler(Server* server, ChallengeManager* challenge_manager)
    : server_(server), challenge_manager_(challenge_manager) {
    std::cout << "[CHALLENGE_HANDLER] Initialized" << std::endl;
}

bool ChallengeHandler::canHandle(MessageType type) const {
    return type == CHALLENGE_SEND || type == CHALLENGE_RESPONSE;
}

bool ChallengeHandler::handleMessage(ClientConnection* client,
                                    const MessageHeader& header,
                                    const std::string& payload) {
    std::cout << "[CHALLENGE_HANDLER] Handling message type=" << static_cast<int>(header.type) << std::endl;

    MessageType type = static_cast<MessageType>(header.type);

    switch (type) {
        case CHALLENGE_SEND:
            return handleChallengeSend(client, payload);

        case CHALLENGE_RESPONSE:
            return handleChallengeResponse(client, payload);

        default:
            std::cerr << "[CHALLENGE_HANDLER] Unknown message type: " << static_cast<int>(header.type) << std::endl;
            return false;
    }
}

bool ChallengeHandler::handleChallengeSend(ClientConnection* client, const std::string& payload) {
    // Check if client is authenticated
    if (!client->isAuthenticated()) {
        std::cerr << "[CHALLENGE_HANDLER] Client not authenticated" << std::endl;
        return false;
    }

    uint32_t challenger_id = client->getUserId();

    // Deserialize challenge request
    ChallengeRequest request;
    if (!deserialize(payload, request)) {
        std::cerr << "[CHALLENGE_HANDLER] Failed to deserialize ChallengeRequest" << std::endl;
        return false;
    }

    std::cout << "[CHALLENGE_HANDLER] Challenge send: user " << challenger_id
              << " -> user " << request.target_user_id << std::endl;

    // Send challenge through ChallengeManager
    return challenge_manager_->sendChallenge(challenger_id, request);
}

bool ChallengeHandler::handleChallengeResponse(ClientConnection* client, const std::string& payload) {
    // Check if client is authenticated
    if (!client->isAuthenticated()) {
        std::cerr << "[CHALLENGE_HANDLER] Client not authenticated" << std::endl;
        return false;
    }

    uint32_t responder_id = client->getUserId();

    // Deserialize challenge response
    ChallengeResponse response;
    if (!deserialize(payload, response)) {
        std::cerr << "[CHALLENGE_HANDLER] Failed to deserialize ChallengeResponse" << std::endl;
        return false;
    }

    std::cout << "[CHALLENGE_HANDLER] Challenge response: challenge_id=" << response.challenge_id
              << " accepted=" << response.accepted
              << " from user " << responder_id << std::endl;

    // Respond to challenge through ChallengeManager
    return challenge_manager_->respondToChallenge(responder_id, response);
}
