#ifndef MATCHMAKING_MESSAGES_H
#define MATCHMAKING_MESSAGES_H

#include <cstdint>
#include <cstring>
#include "protocol.h"

/**
 * Matchmaking Message Structures
 * Used for player listing, challenging, and match creation
 */

// ============== PLAYER LIST ==============

struct PlayerInfo_Message {
    uint32_t user_id;
    char username[32];
    char display_name[64];
    int32_t elo_rating;
    PlayerStatus status;    // ONLINE, AVAILABLE, IN_GAME, BUSY

    PlayerInfo_Message()
        : user_id(0),
          elo_rating(1000),
          status(STATUS_OFFLINE) {
        std::memset(username, 0, sizeof(username));
        std::memset(display_name, 0, sizeof(display_name));
    }
} __attribute__((packed));

struct PlayerListRequest {
    // No payload needed, just request
    PlayerListRequest() {}
} __attribute__((packed));

struct PlayerListResponse {
    uint32_t count;         // Number of players
    PlayerInfo_Message players[100];  // Max 100 players in one message

    PlayerListResponse()
        : count(0) {
        std::memset(players, 0, sizeof(players));
    }
} __attribute__((packed));

// ============== PLAYER STATUS UPDATE ==============

struct PlayerStatusUpdate {
    uint32_t user_id;
    PlayerStatus status;
    char display_name[64];
    int32_t elo_rating;

    PlayerStatusUpdate()
        : user_id(0),
          status(STATUS_OFFLINE),
          elo_rating(1000) {
        std::memset(display_name, 0, sizeof(display_name));
    }
} __attribute__((packed));

// ============== CHALLENGE ==============

struct ChallengeRequest {
    uint32_t target_user_id;    // Who to challenge
    uint32_t time_limit;        // Seconds per turn (0 = unlimited)
    bool random_placement;      // Auto-place ships?

    ChallengeRequest()
        : target_user_id(0),
          time_limit(60),
          random_placement(false) {
    }
} __attribute__((packed));

struct ChallengeReceived {
    uint32_t challenge_id;
    uint32_t challenger_id;
    char challenger_name[64];
    int32_t challenger_elo;
    uint32_t time_limit;
    bool random_placement;
    uint64_t expires_at;        // Unix timestamp

    ChallengeReceived()
        : challenge_id(0),
          challenger_id(0),
          challenger_elo(1000),
          time_limit(60),
          random_placement(false),
          expires_at(0) {
        std::memset(challenger_name, 0, sizeof(challenger_name));
    }
} __attribute__((packed));

struct ChallengeResponse {
    uint32_t challenge_id;
    bool accepted;

    ChallengeResponse()
        : challenge_id(0),
          accepted(false) {
    }
} __attribute__((packed));

struct ChallengeResult {
    uint32_t challenge_id;
    bool success;
    uint32_t match_id;      // If accepted
    char error_message[128];

    ChallengeResult()
        : challenge_id(0),
          success(false),
          match_id(0) {
        std::memset(error_message, 0, sizeof(error_message));
    }
} __attribute__((packed));

// ============== MATCH START ==============

struct MatchStartMessage {
    uint32_t match_id;
    uint32_t opponent_id;
    char opponent_name[64];
    int32_t opponent_elo;
    uint32_t time_limit;
    bool you_go_first;

    MatchStartMessage()
        : match_id(0),
          opponent_id(0),
          opponent_elo(1000),
          time_limit(60),
          you_go_first(false) {
        std::memset(opponent_name, 0, sizeof(opponent_name));
    }
} __attribute__((packed));

struct MatchReadyMessage {
    uint32_t match_id;

    MatchReadyMessage() {
        match_id = 0;
    }
} __attribute__((packed));

#endif // MATCHMAKING_MESSAGES_H
