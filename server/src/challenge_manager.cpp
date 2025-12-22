#include "challenge_manager.h"
#include "server.h"
#include "player_manager.h"
#include "client_connection.h"
#include "database.h"
#include "message_serialization.h"
#include <iostream>
#include <cstring>

using namespace MessageSerialization;

ChallengeManager::ChallengeManager(Server* server, PlayerManager* player_manager)
    : server_(server), player_manager_(player_manager), next_challenge_id_(1) {
    std::cout << "[CHALLENGE_MANAGER] Initialized" << std::endl;
}

ChallengeManager::~ChallengeManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    challenges_.clear();
}

bool ChallengeManager::sendChallenge(uint32_t challenger_id, const ChallengeRequest& request) {
    std::string error;

    // Validate challenge
    if (!validateChallenge(challenger_id, request.target_user_id, error)) {
        std::cerr << "[CHALLENGE] Validation failed: " << error << std::endl;

        // Send error to challenger
        ChallengeResult result;
        result.success = false;
        safeStrCopy(result.error_message, error, sizeof(result.error_message));

        ClientConnection* challenger = player_manager_->getClientConnection(challenger_id);
        if (challenger) {
            MessageHeader header;
            header.type = CHALLENGE_RESPONSE;
            challenger->sendMessage(header, serialize(result));
        }

        return false;
    }

    // Create pending challenge
    PendingChallenge challenge;
    time_t now = time(nullptr);

    {
        std::lock_guard<std::mutex> lock(mutex_);

        challenge.challenge_id = next_challenge_id_++;
        challenge.challenger_id = challenger_id;
        challenge.target_id = request.target_user_id;
        challenge.time_limit = request.time_limit;
        challenge.random_placement = request.random_placement;
        challenge.created_at = now;
        challenge.expires_at = now + CHALLENGE_TIMEOUT_SECONDS;

        challenges_[challenge.challenge_id] = challenge;
    }

    std::cout << "[CHALLENGE] Challenge #" << challenge.challenge_id
              << " sent from user " << challenger_id
              << " to user " << request.target_user_id << std::endl;

    // Notify target player
    notifyTarget(challenge);

    return true;
}

bool ChallengeManager::respondToChallenge(uint32_t responder_id, const ChallengeResponse& response) {
    PendingChallenge challenge;
    bool found = false;

    // Find and remove challenge
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = challenges_.find(response.challenge_id);
        if (it != challenges_.end()) {
            challenge = it->second;

            // Validate responder is the target
            if (challenge.target_id != responder_id) {
                std::cerr << "[CHALLENGE] Invalid responder: expected "
                          << challenge.target_id << ", got " << responder_id << std::endl;
                return false;
            }

            found = true;
            challenges_.erase(it);
        }
    }

    if (!found) {
        std::cout << "[CHALLENGE] Challenge " << response.challenge_id
                  << " not found (possibly cancelled or expired)" << std::endl;

        // Notify responder
        ChallengeResult result;
        result.challenge_id = response.challenge_id;
        result.success = false;
        safeStrCopy(result.error_message, "Challenge not found or expired", sizeof(result.error_message));

        ClientConnection* responder = player_manager_->getClientConnection(responder_id);
        if (responder) {
            MessageHeader header;
            header.type = CHALLENGE_RESPONSE;
            responder->sendMessage(header, serialize(result));
        }

        return false;
    }

    std::cout << "[CHALLENGE] Challenge #" << challenge.challenge_id
              << " " << (response.accepted ? "ACCEPTED" : "DECLINED")
              << " by user " << responder_id << std::endl;

    if (response.accepted) {
        // Create match
        uint32_t match_id = createMatch(challenge);

        if (match_id > 0) {
            // Notify both players of match creation
            PlayerInfo_Message challenger_info = player_manager_->getPlayerInfo(challenge.challenger_id);
            PlayerInfo_Message target_info = player_manager_->getPlayerInfo(challenge.target_id);

            // Send to challenger
            MatchStartMessage match_msg_challenger;
            match_msg_challenger.match_id = match_id;
            match_msg_challenger.opponent_id = challenge.target_id;
            safeStrCopy(match_msg_challenger.opponent_name, target_info.display_name, sizeof(match_msg_challenger.opponent_name));
            match_msg_challenger.opponent_elo = target_info.elo_rating;
            match_msg_challenger.time_limit = challenge.time_limit;
            match_msg_challenger.you_go_first = true;  // Challenger goes first

            ClientConnection* challenger = player_manager_->getClientConnection(challenge.challenger_id);
            if (challenger) {
                MessageHeader header;
                memset(&header, 0, sizeof(header));
                header.type = MATCH_START;
                header.length = sizeof(match_msg_challenger);
                header.timestamp = time(nullptr);

                std::string payload = serialize(match_msg_challenger);
                challenger->sendMessage(header, payload);
                std::cout << "[CHALLENGE] Sent MATCH_START to challenger (user_id=" << challenge.challenger_id << ")" << std::endl;
            }

            // Send to target
            MatchStartMessage match_msg_target;
            match_msg_target.match_id = match_id;
            match_msg_target.opponent_id = challenge.challenger_id;
            safeStrCopy(match_msg_target.opponent_name, challenger_info.display_name, sizeof(match_msg_target.opponent_name));
            match_msg_target.opponent_elo = challenger_info.elo_rating;
            match_msg_target.time_limit = challenge.time_limit;
            match_msg_target.you_go_first = false;  // Target goes second

            ClientConnection* target = player_manager_->getClientConnection(challenge.target_id);
            if (target) {
                MessageHeader header;
                memset(&header, 0, sizeof(header));
                header.type = MATCH_START;
                header.length = sizeof(match_msg_target);
                header.timestamp = time(nullptr);

                std::string payload = serialize(match_msg_target);
                target->sendMessage(header, payload);
                std::cout << "[CHALLENGE] Sent MATCH_START to target (user_id=" << challenge.target_id << ")" << std::endl;
            }

            // Update player statuses to IN_GAME
            player_manager_->updatePlayerStatus(challenge.challenger_id, STATUS_IN_GAME);
            player_manager_->updatePlayerStatus(challenge.target_id, STATUS_IN_GAME);

            std::cout << "[CHALLENGE] Match #" << match_id << " created successfully" << std::endl;
        } else {
            notifyChallenger(challenge, false, "Failed to create match");
        }
    } else {
        // Declined
        notifyChallenger(challenge, false, "Challenge declined");
    }

    return true;
}

void ChallengeManager::cancelChallenge(uint32_t challenge_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    challenges_.erase(challenge_id);
    std::cout << "[CHALLENGE] Challenge #" << challenge_id << " cancelled" << std::endl;
}

void ChallengeManager::checkExpiredChallenges() {
    time_t now = time(nullptr);
    std::vector<PendingChallenge> expired;

    // Find expired challenges
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = challenges_.begin(); it != challenges_.end(); ) {
            if (it->second.expires_at <= now) {
                expired.push_back(it->second);
                it = challenges_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Notify players of expired challenges
    for (const auto& challenge : expired) {
        std::cout << "[CHALLENGE] Challenge #" << challenge.challenge_id << " expired" << std::endl;
        notifyChallenger(challenge, false, "Challenge timed out");

        // Notify target as well
        ChallengeResult result;
        result.challenge_id = challenge.challenge_id;
        result.success = false;
        safeStrCopy(result.error_message, "Challenge timed out", sizeof(result.error_message));

        ClientConnection* target = player_manager_->getClientConnection(challenge.target_id);
        if (target) {
            MessageHeader header;
            header.type = CHALLENGE_RESPONSE;
            target->sendMessage(header, serialize(result));
        }
    }
}

void ChallengeManager::removePlayerChallenges(uint32_t user_id) {
    std::vector<uint32_t> to_remove;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& pair : challenges_) {
            if (pair.second.challenger_id == user_id || pair.second.target_id == user_id) {
                to_remove.push_back(pair.first);
            }
        }

        for (uint32_t challenge_id : to_remove) {
            challenges_.erase(challenge_id);
        }
    }

    if (!to_remove.empty()) {
        std::cout << "[CHALLENGE] Removed " << to_remove.size()
                  << " challenges for user " << user_id << std::endl;
    }
}

bool ChallengeManager::validateChallenge(uint32_t challenger_id, uint32_t target_id, std::string& error) {
    // Check if target exists and is online
    if (!player_manager_->isPlayerOnline(target_id)) {
        error = "Target player is not online";
        return false;
    }

    // Check if target is available
    PlayerStatus target_status = player_manager_->getPlayerStatus(target_id);
    if (target_status != STATUS_AVAILABLE) {
        if (target_status == STATUS_IN_GAME) {
            error = "Target player is already in a game";
        } else if (target_status == STATUS_BUSY) {
            error = "Target player is busy";
        } else {
            error = "Target player is not available";
        }
        return false;
    }

    // Check if challenger is available
    PlayerStatus challenger_status = player_manager_->getPlayerStatus(challenger_id);
    if (challenger_status != STATUS_AVAILABLE) {
        error = "You must be available to send a challenge";
        return false;
    }

    // Check if not challenging self
    if (challenger_id == target_id) {
        error = "Cannot challenge yourself";
        return false;
    }

    return true;
}

uint32_t ChallengeManager::createMatch(const PendingChallenge& challenge) {
    if (!server_ || !server_->getDatabase()) {
        std::cerr << "[CHALLENGE] No database available" << std::endl;
        return 0;
    }

    DatabaseManager* db = server_->getDatabase();

    // Create match in database
    uint32_t match_id = db->createMatch(challenge.challenger_id, challenge.target_id);

    if (match_id == 0) {
        std::cerr << "[CHALLENGE] Failed to create match in database: " << db->getLastError() << std::endl;
        return 0;
    }

    return match_id;
}

void ChallengeManager::notifyChallenger(const PendingChallenge& challenge, bool accepted, const std::string& error) {
    ChallengeResult result;
    result.challenge_id = challenge.challenge_id;
    result.success = accepted;

    if (!error.empty()) {
        safeStrCopy(result.error_message, error, sizeof(result.error_message));
    }

    ClientConnection* challenger = player_manager_->getClientConnection(challenge.challenger_id);
    if (challenger) {
        MessageHeader header;
        memset(&header, 0, sizeof(header));
        header.type = CHALLENGE_RESPONSE;
        header.length = sizeof(result);
        header.timestamp = time(nullptr);

        std::string payload = serialize(result);
        challenger->sendMessage(header, payload);
    }
}

void ChallengeManager::notifyTarget(const PendingChallenge& challenge) {
    PlayerInfo_Message challenger_info = player_manager_->getPlayerInfo(challenge.challenger_id);

    ChallengeReceived msg;
    msg.challenge_id = challenge.challenge_id;
    msg.challenger_id = challenge.challenger_id;
    safeStrCopy(msg.challenger_name, challenger_info.display_name, sizeof(msg.challenger_name));
    msg.challenger_elo = challenger_info.elo_rating;
    msg.time_limit = challenge.time_limit;
    msg.random_placement = challenge.random_placement;
    msg.expires_at = challenge.expires_at;

    ClientConnection* target = player_manager_->getClientConnection(challenge.target_id);
    std::cout << "[CHALLENGE] notifyTarget: target_id=" << challenge.target_id
              << ", target=" << (void*)target << std::endl;
    if (target) {
        MessageHeader header;
        memset(&header, 0, sizeof(header));
        header.type = CHALLENGE_RECEIVED;
        header.length = sizeof(msg);
        header.timestamp = time(nullptr);

        std::string payload = serialize(msg);
        std::cout << "[CHALLENGE] Sending CHALLENGE_RECEIVED to target (payload size=" << payload.size() << ")" << std::endl;
        target->sendMessage(header, payload);
    } else {
        std::cerr << "[CHALLENGE] Target client not found: user_id=" << challenge.target_id << std::endl;
    }
}
