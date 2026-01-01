#ifndef GAMEPLAY_HANDLER_H
#define GAMEPLAY_HANDLER_H

#include "message_handler.h"
#include "database.h"
#include "game_state.h"
#include "messages/gameplay_messages.h"
#include <map>
#include <set>
#include <mutex>
#include <memory>

class Server;

/**
 * GameplayHandler - Handles all gameplay-related messages
 * - Ship placement and validation
 * - Move processing and hit detection
 * - Turn management
 * - Match end and results
 */
class GameplayHandler : public MessageHandler {
private:
    Server* server_;
    DatabaseManager* db_;

    // Active matches (match_id -> MatchState)
    std::map<uint32_t, std::shared_ptr<MatchState>> active_matches_;
    std::mutex matches_mutex_;

    // Track which players are ready (match_id -> set of ready player_ids)
    std::map<uint32_t, std::set<uint32_t>> ready_players_;
    std::mutex ready_mutex_;

public:
    GameplayHandler(Server* server, DatabaseManager* db);
    ~GameplayHandler();

    // MessageHandler interface
    bool handleMessage(ClientConnection* client,
                      const MessageHeader& header,
                      const std::string& payload) override;
    bool canHandle(MessageType type) const override;

    // Specific handlers
    void handleShipPlacement(const MessageHeader& header, const ShipPlacementMessage& msg, int client_fd);
    void handleMove(const MessageHeader& header, const MoveMessage& msg, int client_fd);
    void handleResign(const MessageHeader& header, const ResignMessage& msg, int client_fd);
    void handleDrawOffer(const MessageHeader& header, const DrawOfferMessage& msg, int client_fd);
    void handleDrawResponse(const MessageHeader& header, const DrawResponseMessage& msg, int client_fd);

    // Match management
    std::shared_ptr<MatchState> getMatch(uint32_t match_id);
    void createMatch(uint32_t match_id, uint32_t player1_id, uint32_t player2_id);
    void removeMatch(uint32_t match_id);

    // Validation
    bool validateShipPlacement(const Ship ships[5]);

    // Notifications
    void sendMatchReady(uint32_t match_id, uint32_t player1_id, uint32_t player2_id);
    void sendMoveResult(uint32_t match_id, uint32_t shooter_id, uint32_t target_id,
                       const Coordinate& target, ShotResult result, ShipType ship_sunk,
                       uint32_t ships_remaining, bool game_over, uint32_t winner_id);
    void sendTurnUpdate(uint32_t match_id, uint32_t current_player_id, uint32_t turn_number);
    void sendMatchEnd(uint32_t match_id, uint32_t player1_id, uint32_t player2_id,
                     uint32_t winner_id, MatchEndReason reason, const char* reason_text,
                     uint32_t total_moves, uint64_t duration);

    // Handle player disconnect during match
    void handlePlayerDisconnect(uint32_t disconnected_user_id);
};

#endif // GAMEPLAY_HANDLER_H
