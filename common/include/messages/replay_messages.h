#ifndef REPLAY_MESSAGES_H
#define REPLAY_MESSAGES_H

#include <cstdint>
#include <cstring>
#include "protocol.h"

/**
 * Replay Message Structures
 * Used for match history and replay functionality
 */

// ============== REPLAY REQUEST ==============

struct ReplayListRequest {
    uint32_t user_id;       // Get replays for this user (0 = all your matches)
    uint32_t limit;         // Max number of results (default 20)
    uint32_t offset;        // Pagination offset

    ReplayListRequest()
        : user_id(0),
          limit(20),
          offset(0) {
    }
} __attribute__((packed));

struct ReplayMatchInfo {
    uint32_t match_id;
    uint32_t player1_id;
    uint32_t player2_id;
    uint32_t winner_id;     // 0 = draw
    char player1_name[64];
    char player2_name[64];
    int32_t player1_elo_before;
    int32_t player2_elo_before;
    int32_t player1_elo_after;
    int32_t player2_elo_after;
    uint64_t created_at;    // Unix timestamp
    uint64_t ended_at;      // Unix timestamp
    uint32_t duration_seconds;
    uint32_t total_moves;
    char end_reason[64];    // "normal", "resign", "timeout", etc.

    ReplayMatchInfo()
        : match_id(0),
          player1_id(0),
          player2_id(0),
          winner_id(0),
          player1_elo_before(1000),
          player2_elo_before(1000),
          player1_elo_after(1000),
          player2_elo_after(1000),
          created_at(0),
          ended_at(0),
          duration_seconds(0),
          total_moves(0) {
        std::memset(player1_name, 0, sizeof(player1_name));
        std::memset(player2_name, 0, sizeof(player2_name));
        std::memset(end_reason, 0, sizeof(end_reason));
    }
} __attribute__((packed));

struct ReplayListResponse {
    uint32_t total_count;   // Total matches available
    uint32_t count;         // Number of matches in this response
    ReplayMatchInfo matches[50];  // Max 50 matches per response

    ReplayListResponse()
        : total_count(0),
          count(0) {
    }
} __attribute__((packed));

// ============== DETAILED REPLAY ==============

struct ReplayMoveInfo {
    uint32_t move_number;
    uint32_t player_id;
    Coordinate target;
    ShotResult result;
    uint64_t timestamp;

    ReplayMoveInfo()
        : move_number(0),
          player_id(0),
          result(SHOT_MISS),
          timestamp(0) {
        target.row = 0;
        target.col = 0;
    }
} __attribute__((packed));

struct ReplayDetailRequest {
    uint32_t match_id;

    ReplayDetailRequest()
        : match_id(0) {
    }
} __attribute__((packed));

struct ReplayDetailResponse {
    uint32_t match_id;
    Ship player1_ships[5];
    Ship player2_ships[5];
    uint32_t move_count;
    ReplayMoveInfo moves[200];  // Max 200 moves (100 per player)

    ReplayDetailResponse()
        : match_id(0),
          move_count(0) {
    }
} __attribute__((packed));

#endif // REPLAY_MESSAGES_H
