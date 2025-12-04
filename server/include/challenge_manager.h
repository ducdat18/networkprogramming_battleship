#ifndef CHALLENGE_MANAGER_H
#define CHALLENGE_MANAGER_H

#include <map>
#include <mutex>
#include <cstdint>
#include <ctime>
#include "protocol.h"
#include "messages/matchmaking_messages.h"

// Forward declarations
class ClientConnection;
class Server;
class PlayerManager;

/**
 * ChallengeManager - Manages challenges between players
 *
 * Responsibilities:
 * - Track pending challenges with timeout
 * - Validate challenge requests (target available, not busy)
 * - Route challenges between players
 * - Handle challenge responses (accept/decline)
 * - Create matches on acceptance
 * - Auto-decline on timeout (60s)
 */
class ChallengeManager {
public:
    ChallengeManager(Server* server, PlayerManager* player_manager);
    ~ChallengeManager();

    // Challenge operations
    bool sendChallenge(uint32_t challenger_id, const ChallengeRequest& request);
    bool respondToChallenge(uint32_t responder_id, const ChallengeResponse& response);
    void cancelChallenge(uint32_t challenge_id);

    // Timeout handling
    void checkExpiredChallenges();

    // Cleanup
    void removePlayerChallenges(uint32_t user_id);

private:
    struct PendingChallenge {
        uint32_t challenge_id;
        uint32_t challenger_id;
        uint32_t target_id;
        uint32_t time_limit;
        bool random_placement;
        time_t created_at;
        time_t expires_at;

        PendingChallenge()
            : challenge_id(0), challenger_id(0), target_id(0),
              time_limit(60), random_placement(false),
              created_at(0), expires_at(0) {}
    };

    // Validation
    bool validateChallenge(uint32_t challenger_id, uint32_t target_id, std::string& error);

    // Match creation
    uint32_t createMatch(const PendingChallenge& challenge);

    // Notification
    void notifyChallenger(const PendingChallenge& challenge, bool accepted, const std::string& error = "");
    void notifyTarget(const PendingChallenge& challenge);

    Server* server_;
    PlayerManager* player_manager_;
    mutable std::mutex mutex_;

    std::map<uint32_t, PendingChallenge> challenges_;  // challenge_id -> PendingChallenge
    uint32_t next_challenge_id_;

    static constexpr time_t CHALLENGE_TIMEOUT_SECONDS = 60;
};

#endif // CHALLENGE_MANAGER_H
