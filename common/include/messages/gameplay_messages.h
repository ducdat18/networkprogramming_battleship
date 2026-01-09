#ifndef GAMEPLAY_MESSAGES_H
#define GAMEPLAY_MESSAGES_H

#include <cstdint>
#include <cstring>
#include "protocol.h"

/**
 * Gameplay Message Structures
 * Used during active game play
 */

// ============== SHIP PLACEMENT ==============

struct ShipPlacementMessage {
    uint32_t match_id;
    Ship ships[5];          // All 5 ships
    bool ready;             // True when placement confirmed

    ShipPlacementMessage()
        : match_id(0),
          ready(false) {
        std::memset(ships, 0, sizeof(ships));
    }
} __attribute__((packed));

struct ShipPlacementAck {
    uint32_t match_id;
    bool valid;
    char error_message[128];

    ShipPlacementAck()
        : match_id(0),
          valid(false) {
        std::memset(error_message, 0, sizeof(error_message));
    }
} __attribute__((packed));

// ============== MOVE/TURN ==============

struct MoveMessage {
    uint32_t match_id;
    Coordinate target;

    MoveMessage()
        : match_id(0) {
        target.row = 0;
        target.col = 0;
    }
} __attribute__((packed));

struct MoveResultMessage {
    uint32_t match_id;
    uint32_t shooter_id;    // Who made this shot
    Coordinate target;
    ShotResult result;      // MISS, HIT, SUNK
    ShipType ship_sunk;     // If result == SUNK
    uint32_t ships_remaining;
    bool game_over;
    uint32_t winner_id;     // If game_over

    MoveResultMessage()
        : match_id(0),
          shooter_id(0),
          result(SHOT_MISS),
          ship_sunk(SHIP_CARRIER),
          ships_remaining(5),
          game_over(false),
          winner_id(0) {
        target.row = 0;
        target.col = 0;
    }
} __attribute__((packed));

struct TurnUpdateMessage {
    uint32_t match_id;
    uint32_t current_player_id;
    uint32_t turn_number;
    uint32_t time_left;     // Seconds remaining for turn

    TurnUpdateMessage()
        : match_id(0),
          current_player_id(0),
          turn_number(1),
          time_left(60) {
    }
} __attribute__((packed));

struct TurnTimeoutMessage {
    uint32_t match_id;

    TurnTimeoutMessage() : match_id(0) {}
} __attribute__((packed));

// ============== MATCH STATE ==============

struct MatchStateMessage {
    uint32_t match_id;
    uint32_t player1_id;
    uint32_t player2_id;
    uint32_t current_turn_player_id;
    uint32_t turn_number;
    bool is_active;
    bool is_paused;

    MatchStateMessage()
        : match_id(0),
          player1_id(0),
          player2_id(0),
          current_turn_player_id(0),
          turn_number(1),
          is_active(false),
          is_paused(false) {
    }
} __attribute__((packed));

// ============== MATCH END ==============

struct MatchEndMessage {
    uint32_t match_id;
    GameResult result;      // WIN, LOSS, DRAW
    uint32_t winner_id;
    MatchEndReason reason;  // Why match ended
    int32_t elo_change;     // + or - ELO points
    int32_t new_elo;
    uint32_t total_moves;
    uint64_t duration;      // Seconds
    char reason_text[128];  // Human-readable reason

    MatchEndMessage()
        : match_id(0),
          result(RESULT_DRAW),
          winner_id(0),
          reason(END_NORMAL),
          elo_change(0),
          new_elo(1000),
          total_moves(0),
          duration(0) {
        std::memset(reason_text, 0, sizeof(reason_text));
    }
} __attribute__((packed));

// ============== MATCH ACTIONS ==============

struct PauseRequestMessage {
    uint32_t match_id;

    PauseRequestMessage() {
        match_id = 0;
    }
} __attribute__((packed));

struct PauseResponseMessage {
    uint32_t match_id;
    bool accepted;

    PauseResponseMessage() {
        match_id = 0;
        accepted = false;
    }
} __attribute__((packed));

struct DrawOfferMessage {
    uint32_t match_id;

    DrawOfferMessage() {
        match_id = 0;
    }
} __attribute__((packed));

struct DrawResponseMessage {
    uint32_t match_id;
    bool accepted;

    DrawResponseMessage() {
        match_id = 0;
        accepted = false;
    }
} __attribute__((packed));

struct ResignMessage {
    uint32_t match_id;

    ResignMessage() {
        match_id = 0;
    }
} __attribute__((packed));

struct RematchRequestMessage {
    uint32_t previous_match_id;

    RematchRequestMessage() {
        previous_match_id = 0;
    }
} __attribute__((packed));

struct RematchResponseMessage {
    uint32_t previous_match_id;
    bool accepted;
    uint32_t new_match_id;  // If accepted

    RematchResponseMessage() {
        memset(this, 0, sizeof(RematchResponseMessage));
        previous_match_id = 0;
        accepted = false;
        new_match_id = 0;
    }
} __attribute__((packed));

// ============== CHAT ==============

struct ChatMessage {
    uint32_t match_id;
    char message[256];

    ChatMessage() {
        memset(this, 0, sizeof(ChatMessage));
        match_id = 0;
    }
} __attribute__((packed));

#endif // GAMEPLAY_MESSAGES_H
