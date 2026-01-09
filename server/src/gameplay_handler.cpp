#include "gameplay_handler.h"
#include "server.h"
#include "player_manager.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <cmath>

GameplayHandler::GameplayHandler(Server* server, DatabaseManager* db)
    : server_(server), db_(db) {
}

GameplayHandler::~GameplayHandler() {
}

bool GameplayHandler::canHandle(MessageType type) const {
    return (type == MessageType::SHIP_PLACEMENT ||
            type == MessageType::MOVE ||
            type == MessageType::TURN_TIMEOUT ||
            type == MessageType::RESIGN ||
            type == MessageType::DRAW_OFFER ||
            type == MessageType::DRAW_RESPONSE ||
            type == MessageType::REMATCH_REQUEST ||
            type == MessageType::REMATCH_RESPONSE);
}

bool GameplayHandler::handleMessage(ClientConnection* client,
                                    const MessageHeader& header,
                                    const std::string& payload) {
    if (!client) return false;

    int client_fd = client->getSocketFd();
    MessageType type = static_cast<MessageType>(header.type);

    switch (type) {
        case MessageType::SHIP_PLACEMENT: {
            if (payload.size() >= sizeof(ShipPlacementMessage)) {
                ShipPlacementMessage msg;
                memcpy(&msg, payload.data(), sizeof(ShipPlacementMessage));
                handleShipPlacement(header, msg, client_fd);
                return true;
            }
            break;
        }

        case MessageType::MOVE: {
            if (payload.size() >= sizeof(MoveMessage)) {
                MoveMessage msg;
                memcpy(&msg, payload.data(), sizeof(MoveMessage));
                handleMove(header, msg, client_fd);
                return true;
            }
            break;
        }

        case MessageType::TURN_TIMEOUT: {
            if (payload.size() >= sizeof(TurnTimeoutMessage)) {
                TurnTimeoutMessage msg;
                memcpy(&msg, payload.data(), sizeof(TurnTimeoutMessage));
                handleTurnTimeout(header, msg, client_fd);
                return true;
            }
            break;
        }

        case MessageType::RESIGN: {
            if (payload.size() >= sizeof(ResignMessage)) {
                ResignMessage msg;
                memcpy(&msg, payload.data(), sizeof(ResignMessage));
                handleResign(header, msg, client_fd);
                return true;
            }
            break;
        }

        case MessageType::DRAW_OFFER: {
            if (payload.size() >= sizeof(DrawOfferMessage)) {
                DrawOfferMessage msg;
                memcpy(&msg, payload.data(), sizeof(DrawOfferMessage));
                handleDrawOffer(header, msg, client_fd);
                return true;
            }
            break;
        }

        case MessageType::DRAW_RESPONSE: {
            if (payload.size() >= sizeof(DrawResponseMessage)) {
                DrawResponseMessage msg;
                memcpy(&msg, payload.data(), sizeof(DrawResponseMessage));
                handleDrawResponse(header, msg, client_fd);
                return true;
            }
            break;
        }

        case MessageType::REMATCH_REQUEST: {
            if (payload.size() >= sizeof(RematchRequestMessage)) {
                RematchRequestMessage msg;
                memcpy(&msg, payload.data(), sizeof(RematchRequestMessage));
                handleRematchRequest(header, msg, client_fd);
                return true;
            }
            break;
        }

        case MessageType::REMATCH_RESPONSE: {
            if (payload.size() >= sizeof(RematchResponseMessage)) {
                RematchResponseMessage msg;
                memcpy(&msg, payload.data(), sizeof(RematchResponseMessage));
                handleRematchResponse(header, msg, client_fd);
                return true;
            }
            break;
        }

        default:
            return false;
    }

    return false;
}

void GameplayHandler::handleShipPlacement(const MessageHeader& header,
                                         const ShipPlacementMessage& msg,
                                         int client_fd) {
    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) {
        std::cout << "Invalid session for ship placement" << std::endl;
        return;
    }

    // Validate ship placement
    ShipPlacementAck ack;
    ack.match_id = msg.match_id;

    if (!validateShipPlacement(msg.ships)) {
        ack.valid = false;
        strcpy(ack.error_message, "Invalid ship placement");

        // Send error response
        MessageHeader resp_header;
        memset(&resp_header, 0, sizeof(resp_header));
        resp_header.type = MessageType::SHIP_PLACEMENT;
        resp_header.length = sizeof(ack);
        resp_header.timestamp = time(nullptr);

        server_->sendToClient(client_fd, resp_header, &ack, sizeof(ack));
        return;
    }

    // Store ships in database
    std::string ship_data = ""; // TODO: Serialize ships to JSON
    for (int i = 0; i < 5; i++) {
        ship_data += std::to_string((int)msg.ships[i].type) + "," +
                    std::to_string((int)msg.ships[i].orientation) + "," +
                    std::to_string((int)msg.ships[i].position.row) + "," +
                    std::to_string((int)msg.ships[i].position.col) + ";";
    }

    if (!db_->saveShipPlacement(msg.match_id, user_id, ship_data)) {
        std::cout << "Failed to save board data for user " << user_id << std::endl;
        ack.valid = false;
        strcpy(ack.error_message, "Failed to save board data");

        MessageHeader resp_header;
        memset(&resp_header, 0, sizeof(resp_header));
        resp_header.type = MessageType::SHIP_PLACEMENT;
        resp_header.length = sizeof(ack);
        resp_header.timestamp = time(nullptr);

        server_->sendToClient(client_fd, resp_header, &ack, sizeof(ack));
        return;
    }

    // Success acknowledgment
    ack.valid = true;
    strcpy(ack.error_message, "Ship placement accepted");

    MessageHeader resp_header;
    memset(&resp_header, 0, sizeof(resp_header));
    resp_header.type = MessageType::SHIP_PLACEMENT;
    resp_header.length = sizeof(ack);
    resp_header.timestamp = time(nullptr);

    server_->sendToClient(client_fd, resp_header, &ack, sizeof(ack));

    // Mark player as ready
    {
        std::lock_guard<std::mutex> lock(ready_mutex_);
        ready_players_[msg.match_id].insert(user_id);
    }

    // Check if both players are ready
    auto match_data = db_->getMatchById(msg.match_id);
    if (match_data.match_id == 0) {
        std::cout << "Match " << msg.match_id << " not found" << std::endl;
        return;
    }

    uint32_t player1_id = match_data.player1_id;
    uint32_t player2_id = match_data.player2_id;

    bool both_ready = false;
    {
        std::lock_guard<std::mutex> lock(ready_mutex_);
        auto& ready_set = ready_players_[msg.match_id];
        both_ready = (ready_set.count(player1_id) > 0 && ready_set.count(player2_id) > 0);
    }

    if (both_ready) {
        // Create match state
        createMatch(msg.match_id, player1_id, player2_id);

        // Load ships from database for both players
        auto match = getMatch(msg.match_id);
        if (match) {
            // Helper lambda to parse simple "type,orient,row,col;..." format
            auto loadShips = [](Board& board, const std::string& data) {
                if (data.empty()) {
                    return;
                }
                // Start from a clean board
                board.clearBoard();

                std::stringstream ss(data);
                std::string token;
                while (std::getline(ss, token, ';')) {
                    if (token.empty()) continue;

                    std::stringstream ship_ss(token);
                    std::string field;
                    int type_i = 0, orient_i = 0, row = 0, col = 0;

                    // type
                    if (!std::getline(ship_ss, field, ',')) continue;
                    type_i = std::stoi(field);
                    // orientation
                    if (!std::getline(ship_ss, field, ',')) continue;
                    orient_i = std::stoi(field);
                    // row
                    if (!std::getline(ship_ss, field, ',')) continue;
                    row = std::stoi(field);
                    // col
                    if (!std::getline(ship_ss, field, ',')) continue;
                    col = std::stoi(field);

                    ShipType type = static_cast<ShipType>(type_i);
                    Orientation orient = static_cast<Orientation>(orient_i);
                    Coordinate pos;
                    pos.row = row;
                    pos.col = col;

                    // Place ship on board; ignore invalid placements (already validated earlier)
                    board.placeShip(type, pos, orient);
                }
            };

            // Parse and place ships for player 1
            std::string p1_ships = db_->getShipPlacement(msg.match_id, player1_id);
            loadShips(match->player1_board, p1_ships);

            // Parse and place ships for player 2
            std::string p2_ships = db_->getShipPlacement(msg.match_id, player2_id);
            loadShips(match->player2_board, p2_ships);

            // Start the match
            match->startMatch();

            // Randomly select first player
            srand(time(nullptr));
            match->current_turn_player_id = (rand() % 2 == 0) ? player1_id : player2_id;
            match->turn_start_time = time(nullptr);  // Record first turn start time

            // Send MATCH_READY to both players
            sendMatchReady(msg.match_id, player1_id, player2_id);

            // Send initial turn update
            sendTurnUpdate(msg.match_id, match->current_turn_player_id, 1);

            std::cout << "Match " << msg.match_id << " is ready! First turn: " << match->current_turn_player_id << std::endl;
        }
    }
}

void GameplayHandler::handleMove(const MessageHeader& header,
                                const MoveMessage& msg,
                                int client_fd) {
    (void)client_fd;
    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) {
        std::cout << "Invalid session for move" << std::endl;
        return;
    }

    // Get match state
    auto match = getMatch(msg.match_id);
    if (!match) {
        std::cout << "Match " << msg.match_id << " not found" << std::endl;
        return;
    }

    // Validate it's player's turn
    if (match->current_turn_player_id != user_id) {
        std::cout << "Not player " << user_id << "'s turn" << std::endl;
        return;
    }

    // Process the move
    ShotResult result = match->processMove(user_id, msg.target);

    // Get the opponent board to check ships remaining
    Board* target_board = (user_id == match->player1_id) ? &match->player2_board : &match->player1_board;
    uint32_t ships_remaining = target_board->getShipsRemaining();

    // Determine if ship was sunk and which type
    ShipType ship_sunk = SHIP_CARRIER;
    if (result == SHOT_SUNK) {
        // Find which ship was sunk
        const Ship* ships = target_board->getShips();
        for (int i = 0; i < NUM_SHIPS; i++) {
            if (ships[i].is_sunk && ships[i].hits >= ships[i].length) {
                // Check if this ship contains the target coordinate
                for (int j = 0; j < ships[i].length; j++) {
                    int r = ships[i].position.row + (ships[i].orientation == VERTICAL ? j : 0);
                    int c = ships[i].position.col + (ships[i].orientation == HORIZONTAL ? j : 0);
                    if (r == msg.target.row && c == msg.target.col) {
                        ship_sunk = (ShipType)ships[i].type;
                        break;
                    }
                }
            }
        }
    }

    // Check if game is over
    bool game_over = target_board->allShipsSunk();
    uint32_t winner_id = game_over ? user_id : 0;

    // Send move result to both players
    uint32_t opponent_id = (user_id == match->player1_id) ? match->player2_id : match->player1_id;
    sendMoveResult(msg.match_id, user_id, opponent_id, msg.target, result,
                  ship_sunk, ships_remaining, game_over, winner_id);

    // Save move to database
    std::string result_str;
    if (result == SHOT_MISS) result_str = "miss";
    else if (result == SHOT_HIT) result_str = "hit";
    else if (result == SHOT_SUNK) result_str = "sunk";
    db_->saveMove(msg.match_id, user_id, match->turn_number, msg.target.col, msg.target.row, result_str);

    if (game_over) {
        // Calculate duration
        uint64_t duration = time(nullptr) - match->start_time;

        // Send match end with normal completion reason
        sendMatchEnd(msg.match_id, match->player1_id, match->player2_id,
                    winner_id, END_NORMAL, "All ships destroyed",
                    match->move_history.size(), duration);

        // Update match in database
        db_->endMatch(msg.match_id, winner_id);

        // Update player status back to AVAILABLE
        auto player_manager = server_->getPlayerManager();
        if (player_manager) {
            player_manager->updatePlayerStatus(match->player1_id, STATUS_AVAILABLE);
            player_manager->updatePlayerStatus(match->player2_id, STATUS_AVAILABLE);
        }

        // Remove match from active matches
        removeMatch(msg.match_id);
    } else {
        // Send turn update
        sendTurnUpdate(msg.match_id, match->current_turn_player_id, match->turn_number);
    }
}

void GameplayHandler::handleTurnTimeout(const MessageHeader& header,
                                       const TurnTimeoutMessage& msg,
                                       int client_fd) {
    (void)client_fd;

    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) {
        std::cout << "[TIMEOUT] Invalid session" << std::endl;
        return;
    }

    // Get match state
    auto match = getMatch(msg.match_id);
    if (!match) {
        std::cout << "[TIMEOUT] Match " << msg.match_id << " not found" << std::endl;
        return;
    }

    // Validate it's this player's turn
    if (match->current_turn_player_id != user_id) {
        std::cout << "[TIMEOUT] Not player " << user_id << "'s turn" << std::endl;
        return;
    }

    // Server-side validation: check elapsed time
    uint64_t elapsed = time(nullptr) - match->turn_start_time;
    if (elapsed < (uint64_t)match->turn_time_limit) {
        std::cout << "[TIMEOUT] Too early: " << elapsed << "s elapsed, "
                  << match->turn_time_limit << "s required" << std::endl;
        return;
    }

    std::cout << "[TIMEOUT] Player " << user_id << " timed out in match "
              << msg.match_id << " (" << elapsed << "s elapsed)" << std::endl;

    // Switch turn
    match->switchTurn();

    // Broadcast turn update to both players
    sendTurnUpdate(msg.match_id, match->current_turn_player_id, match->turn_number);
}

void GameplayHandler::handleResign(const MessageHeader& header,
                                  const ResignMessage& msg,
                                  int client_fd) {
    (void)client_fd;
    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) {
        return;
    }

    // Get match state
    auto match = getMatch(msg.match_id);
    if (!match) {
        return;
    }

    // Determine winner (opponent)
    uint32_t winner_id = (user_id == match->player1_id) ? match->player2_id : match->player1_id;

    // End match with resign reason
    uint64_t duration = time(nullptr) - match->start_time;
    sendMatchEnd(msg.match_id, match->player1_id, match->player2_id,
                winner_id, END_RESIGN, "Opponent resigned",
                match->move_history.size(), duration);

    // Update database
    db_->endMatch(msg.match_id, winner_id);

    // Update player status
    auto player_manager = server_->getPlayerManager();
    if (player_manager) {
        player_manager->updatePlayerStatus(match->player1_id, STATUS_AVAILABLE);
        player_manager->updatePlayerStatus(match->player2_id, STATUS_AVAILABLE);
    }

    // Remove match
    removeMatch(msg.match_id);
}

void GameplayHandler::handleDrawOffer(const MessageHeader& header,
                                     const DrawOfferMessage& msg,
                                     int client_fd) {
    (void)client_fd;
    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) {
        return;
    }

    // Get match state
    auto match = getMatch(msg.match_id);
    if (!match) {
        return;
    }

    // Forward draw offer to opponent
    uint32_t opponent_id = (user_id == match->player1_id) ? match->player2_id : match->player1_id;
    ClientConnection* opponent_conn = server_->getPlayerManager()->getClientConnection(opponent_id);

    if (opponent_conn) {
        MessageHeader forward_header;
        memset(&forward_header, 0, sizeof(forward_header));
        forward_header.type = MessageType::DRAW_OFFER;
        forward_header.length = sizeof(msg);
        forward_header.timestamp = time(nullptr);

        server_->sendToClient(opponent_conn->getSocketFd(), forward_header, &msg, sizeof(msg));
    }
}

void GameplayHandler::handleDrawResponse(const MessageHeader& header,
                                        const DrawResponseMessage& msg,
                                        int client_fd) {
    (void)client_fd;
    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) {
        return;
    }

    if (!msg.accepted) {
        // Forward decline to opponent
        auto match = getMatch(msg.match_id);
        if (match) {
            uint32_t opponent_id = (user_id == match->player1_id) ? match->player2_id : match->player1_id;
            ClientConnection* opponent_conn = server_->getPlayerManager()->getClientConnection(opponent_id);

            if (opponent_conn) {
                MessageHeader forward_header;
                memset(&forward_header, 0, sizeof(forward_header));
                forward_header.type = MessageType::DRAW_RESPONSE;
                forward_header.length = sizeof(msg);
                forward_header.timestamp = time(nullptr);

                server_->sendToClient(opponent_conn->getSocketFd(), forward_header, &msg, sizeof(msg));
            }
        }
        return;
    }

    // Draw accepted
    auto match = getMatch(msg.match_id);
    if (!match) {
        return;
    }

    // End match as draw (winner_id = 0)
    uint64_t duration = time(nullptr) - match->start_time;
    sendMatchEnd(msg.match_id, match->player1_id, match->player2_id,
                0, END_DRAW_AGREED, "Draw agreed by both players",
                match->move_history.size(), duration);

    // Update database (0 = draw)
    db_->endMatch(msg.match_id, 0);

    // Update player status
    auto player_manager = server_->getPlayerManager();
    if (player_manager) {
        player_manager->updatePlayerStatus(match->player1_id, STATUS_AVAILABLE);
        player_manager->updatePlayerStatus(match->player2_id, STATUS_AVAILABLE);
    }

    // Remove match
    removeMatch(msg.match_id);
}

std::shared_ptr<MatchState> GameplayHandler::getMatch(uint32_t match_id) {
    std::lock_guard<std::mutex> lock(matches_mutex_);
    auto it = active_matches_.find(match_id);
    if (it != active_matches_.end()) {
        return it->second;
    }
    return nullptr;
}

void GameplayHandler::createMatch(uint32_t match_id, uint32_t player1_id, uint32_t player2_id) {
    std::lock_guard<std::mutex> lock(matches_mutex_);

    auto match = std::make_shared<MatchState>();
    match->match_id = std::to_string(match_id);
    match->player1_id = player1_id;
    match->player2_id = player2_id;

    // Load player names from database
    auto p1_info = db_->getUserById(player1_id);
    auto p2_info = db_->getUserById(player2_id);
    match->player1_name = p1_info.username;
    match->player2_name = p2_info.username;

    active_matches_[match_id] = match;
}

void GameplayHandler::removeMatch(uint32_t match_id) {
    {
        std::lock_guard<std::mutex> lock(matches_mutex_);
        active_matches_.erase(match_id);
    }

    {
        std::lock_guard<std::mutex> ready_lock(ready_mutex_);
        ready_players_.erase(match_id);
    }
}

void GameplayHandler::handleRematchRequest(const MessageHeader& header,
                                          const RematchRequestMessage& msg,
                                          int client_fd) {
    // Validate session
    std::string token(header.session_token);
    uint32_t user_id = db_->validateSession(token);
    if (user_id == 0) return;

    // Get match details
    Match match = db_->getMatchById(msg.previous_match_id);
    if (match.match_id == 0) {
        std::cerr << "[GAMEPLAY] Match not found: " << msg.previous_match_id << std::endl;
        return;
    }

    // Determine opponent
    uint32_t opponent_id = (match.player1_id == user_id)
                          ? match.player2_id
                          : match.player1_id;

    if (opponent_id == 0) return;

    // Check opponent is online and available
    auto player_manager = server_->getPlayerManager();
    if (!player_manager) return;

    if (!player_manager->isPlayerOnline(opponent_id)) {
        std::cout << "[GAMEPLAY] Opponent not online for rematch" << std::endl;
        // Send rejection
        RematchResponseMessage response;
        response.previous_match_id = msg.previous_match_id;
        response.accepted = false;
        response.new_match_id = 0;

        MessageHeader resp_header;
        memset(&resp_header, 0, sizeof(resp_header));
        resp_header.type = MessageType::REMATCH_RESPONSE;
        resp_header.length = sizeof(response);
        resp_header.timestamp = time(nullptr);

        server_->sendToClient(client_fd, resp_header, &response, sizeof(response));
        return;
    }

    PlayerStatus opponent_status = player_manager->getPlayerStatus(opponent_id);
    if (opponent_status != STATUS_AVAILABLE) {
        std::cout << "[GAMEPLAY] Opponent not available (status="
                  << opponent_status << ")" << std::endl;
        // Send rejection
        RematchResponseMessage response;
        response.previous_match_id = msg.previous_match_id;
        response.accepted = false;
        response.new_match_id = 0;

        MessageHeader resp_header;
        memset(&resp_header, 0, sizeof(resp_header));
        resp_header.type = MessageType::REMATCH_RESPONSE;
        resp_header.length = sizeof(response);
        resp_header.timestamp = time(nullptr);

        server_->sendToClient(client_fd, resp_header, &response, sizeof(response));
        return;
    }

    // Store pending rematch
    {
        std::lock_guard<std::mutex> lock(rematch_mutex_);
        pending_rematches_[msg.previous_match_id] = {user_id, opponent_id};
    }

    std::cout << "[GAMEPLAY] Rematch request: " << user_id
              << " challenges " << opponent_id << std::endl;

    // Forward request to opponent
    ClientConnection* opponent_conn = player_manager->getClientConnection(opponent_id);
    if (opponent_conn) {
        server_->sendToClient(opponent_conn->getSocketFd(), header, &msg, sizeof(msg));
    }
}

void GameplayHandler::handleRematchResponse(const MessageHeader& header,
                                           const RematchResponseMessage& msg,
                                           int client_fd) {
    (void)client_fd; // Unused parameter

    // Validate session
    std::string token(header.session_token);
    uint32_t responder_id = db_->validateSession(token);
    if (responder_id == 0) return;

    // Get pending rematch
    uint32_t requester_id = 0;
    {
        std::lock_guard<std::mutex> lock(rematch_mutex_);
        auto it = pending_rematches_.find(msg.previous_match_id);
        if (it == pending_rematches_.end()) {
            std::cout << "[GAMEPLAY] No pending rematch found" << std::endl;
            return;
        }

        requester_id = it->first;

        // Clean up if declined
        if (!msg.accepted) {
            pending_rematches_.erase(it);
        }
    }

    auto player_manager = server_->getPlayerManager();
    if (!player_manager) return;

    RematchResponseMessage response = msg;

    if (msg.accepted) {
        std::cout << "[GAMEPLAY] Rematch accepted: creating new match" << std::endl;

        // Create new match
        uint32_t new_match_id = db_->createMatch(requester_id, responder_id);

        if (new_match_id == 0) {
            std::cerr << "[GAMEPLAY] Failed to create rematch" << std::endl;
            response.accepted = false;
            response.new_match_id = 0;
        } else {
            response.new_match_id = new_match_id;

            // Update player statuses
            player_manager->updatePlayerStatus(requester_id, STATUS_IN_GAME);
            player_manager->updatePlayerStatus(responder_id, STATUS_IN_GAME);

            // Send MATCH_START to both players
            User p1 = db_->getUserById(requester_id);
            User p2 = db_->getUserById(responder_id);

            // Send to requester
            MatchStartMessage start_msg1;
            start_msg1.match_id = new_match_id;
            start_msg1.opponent_id = responder_id;
            strncpy(start_msg1.opponent_name, p2.display_name.c_str(), 63);
            start_msg1.opponent_name[63] = '\0';
            start_msg1.opponent_elo = p2.elo_rating;
            start_msg1.time_limit = 60;
            start_msg1.you_go_first = (requester_id == p1.user_id);

            MessageHeader start_header;
            memset(&start_header, 0, sizeof(start_header));
            start_header.type = MessageType::MATCH_START;
            start_header.length = sizeof(start_msg1);
            start_header.timestamp = time(nullptr);

            ClientConnection* req_conn = player_manager->getClientConnection(requester_id);
            if (req_conn) {
                server_->sendToClient(req_conn->getSocketFd(), start_header,
                                     &start_msg1, sizeof(start_msg1));
            }

            // Send to responder
            MatchStartMessage start_msg2 = start_msg1;
            start_msg2.opponent_id = requester_id;
            strncpy(start_msg2.opponent_name, p1.display_name.c_str(), 63);
            start_msg2.opponent_name[63] = '\0';
            start_msg2.opponent_elo = p1.elo_rating;

            ClientConnection* resp_conn = player_manager->getClientConnection(responder_id);
            if (resp_conn) {
                server_->sendToClient(resp_conn->getSocketFd(), start_header,
                                     &start_msg2, sizeof(start_msg2));
            }

            std::cout << "[GAMEPLAY] Rematch started: match_id=" << new_match_id << std::endl;
        }

        // Clean up pending rematch
        std::lock_guard<std::mutex> lock(rematch_mutex_);
        pending_rematches_.erase(msg.previous_match_id);

    } else {
        std::cout << "[GAMEPLAY] Rematch declined" << std::endl;
    }

    // Send response to requester
    ClientConnection* requester_conn = player_manager->getClientConnection(requester_id);
    if (requester_conn) {
        MessageHeader resp_header;
        memset(&resp_header, 0, sizeof(resp_header));
        resp_header.type = MessageType::REMATCH_RESPONSE;
        resp_header.length = sizeof(response);
        resp_header.timestamp = time(nullptr);

        server_->sendToClient(requester_conn->getSocketFd(), resp_header,
                             &response, sizeof(response));
    }
}

bool GameplayHandler::validateShipPlacement(const Ship ships[5]) {
    // Check that we have exactly 5 ships
    int ship_count = 0;
    bool has_carrier = false, has_battleship = false, has_cruiser = false;
    bool has_submarine = false, has_destroyer = false;

    for (int i = 0; i < 5; i++) {
        if (ships[i].length > 0) {
            ship_count++;

            // Check ship type is valid
            switch (ships[i].type) {
                case SHIP_CARRIER: has_carrier = true; break;
                case SHIP_BATTLESHIP: has_battleship = true; break;
                case SHIP_CRUISER: has_cruiser = true; break;
                case SHIP_SUBMARINE: has_submarine = true; break;
                case SHIP_DESTROYER: has_destroyer = true; break;
                default: return false;
            }

            // Check ship length matches type
            int expected_length = getShipLength((ShipType)ships[i].type);
            if (ships[i].length != expected_length) {
                return false;
            }

            // Check position is valid
            if (ships[i].position.row < 0 || ships[i].position.row >= BOARD_SIZE ||
                ships[i].position.col < 0 || ships[i].position.col >= BOARD_SIZE) {
                return false;
            }

            // Check ship fits on board
            if (ships[i].orientation == HORIZONTAL) {
                if (ships[i].position.col + ships[i].length > BOARD_SIZE) {
                    return false;
                }
            } else {
                if (ships[i].position.row + ships[i].length > BOARD_SIZE) {
                    return false;
                }
            }
        }
    }

    // Must have all 5 ship types
    return ship_count == 5 && has_carrier && has_battleship && has_cruiser && has_submarine && has_destroyer;
}

void GameplayHandler::sendMatchReady(uint32_t match_id, uint32_t player1_id, uint32_t player2_id) {
    MatchStateMessage msg;
    msg.match_id = match_id;
    msg.player1_id = player1_id;
    msg.player2_id = player2_id;
    msg.is_active = true;
    msg.turn_number = 1;

    MessageHeader header;
    memset(&header, 0, sizeof(header));
    header.type = MessageType::MATCH_READY;
    header.length = sizeof(msg);
    header.timestamp = time(nullptr);

    // Send to both players
    auto player_manager = server_->getPlayerManager();
    if (player_manager) {
        ClientConnection* p1_conn = player_manager->getClientConnection(player1_id);
        ClientConnection* p2_conn = player_manager->getClientConnection(player2_id);

        if (p1_conn) {
            server_->sendToClient(p1_conn->getSocketFd(), header, &msg, sizeof(msg));
        }
        if (p2_conn) {
            server_->sendToClient(p2_conn->getSocketFd(), header, &msg, sizeof(msg));
        }
    }
}

void GameplayHandler::sendMoveResult(uint32_t match_id, uint32_t shooter_id, uint32_t target_id,
                                     const Coordinate& target, ShotResult result, ShipType ship_sunk,
                                     uint32_t ships_remaining, bool game_over, uint32_t winner_id) {
    MoveResultMessage msg;
    msg.match_id = match_id;
    msg.shooter_id = shooter_id;  // Include shooter ID so clients know whose shot this is
    msg.target = target;
    msg.result = result;
    msg.ship_sunk = ship_sunk;
    msg.ships_remaining = ships_remaining;
    msg.game_over = game_over;
    msg.winner_id = winner_id;

    MessageHeader header;
    memset(&header, 0, sizeof(header));
    header.type = MessageType::MOVE_RESULT;
    header.length = sizeof(msg);
    header.timestamp = time(nullptr);

    // Send to both players
    auto player_manager = server_->getPlayerManager();
    if (player_manager) {
        ClientConnection* shooter_conn = player_manager->getClientConnection(shooter_id);
        ClientConnection* target_conn = player_manager->getClientConnection(target_id);

        if (shooter_conn) {
            server_->sendToClient(shooter_conn->getSocketFd(), header, &msg, sizeof(msg));
        }
        if (target_conn) {
            server_->sendToClient(target_conn->getSocketFd(), header, &msg, sizeof(msg));
        }
    }
}

void GameplayHandler::sendTurnUpdate(uint32_t match_id, uint32_t current_player_id, uint32_t turn_number) {
    TurnUpdateMessage msg;
    msg.match_id = match_id;
    msg.current_player_id = current_player_id;
    msg.turn_number = turn_number;
    msg.time_left = 20;

    MessageHeader header;
    memset(&header, 0, sizeof(header));
    header.type = MessageType::TURN_UPDATE;
    header.length = sizeof(msg);
    header.timestamp = time(nullptr);

    // Get match state to find both players
    auto match = getMatch(match_id);
    if (!match) return;

    // Send to both players
    auto player_manager = server_->getPlayerManager();
    if (player_manager) {
        ClientConnection* p1_conn = player_manager->getClientConnection(match->player1_id);
        ClientConnection* p2_conn = player_manager->getClientConnection(match->player2_id);

        if (p1_conn) {
            server_->sendToClient(p1_conn->getSocketFd(), header, &msg, sizeof(msg));
        }
        if (p2_conn) {
            server_->sendToClient(p2_conn->getSocketFd(), header, &msg, sizeof(msg));
        }
    }
}

void GameplayHandler::sendMatchEnd(uint32_t match_id, uint32_t player1_id, uint32_t player2_id,
                                  uint32_t winner_id, MatchEndReason reason, const char* reason_text,
                                  uint32_t total_moves, uint64_t duration) {
    // Send to player 1
    MatchEndMessage msg1;
    msg1.match_id = match_id;
    msg1.winner_id = winner_id;
    msg1.reason = reason;
    strncpy(msg1.reason_text, reason_text ? reason_text : "", sizeof(msg1.reason_text) - 1);
    msg1.reason_text[sizeof(msg1.reason_text) - 1] = '\0';
    msg1.total_moves = total_moves;
    msg1.duration = duration;
    // Calculate and apply ELO changes (simple Elo system)
    int32_t p1_old_elo = 1000;
    int32_t p2_old_elo = 1000;

    if (db_) {
        User p1 = db_->getUserById(player1_id);
        User p2 = db_->getUserById(player2_id);
        if (p1.user_id == player1_id) p1_old_elo = p1.elo_rating;
        if (p2.user_id == player2_id) p2_old_elo = p2.elo_rating;
    }

    // Elo calculation
    auto computeElo = [](int32_t ra, int32_t rb, double score, int k = 30) -> int32_t {
        double ea = 1.0 / (1.0 + std::pow(10.0, (rb - ra) / 400.0));
        double new_ra = ra + k * (score - ea);
        return static_cast<int32_t>(std::round(new_ra));
    };

    int32_t p1_new_elo = p1_old_elo;
    int32_t p2_new_elo = p2_old_elo;
    int32_t p1_change = 0;
    int32_t p2_change = 0;

    if (winner_id == 0) {
        // Draw: both get 0.5
        p1_new_elo = computeElo(p1_old_elo, p2_old_elo, 0.5);
        p2_new_elo = computeElo(p2_old_elo, p1_old_elo, 0.5);
    } else if (winner_id == player1_id) {
        p1_new_elo = computeElo(p1_old_elo, p2_old_elo, 1.0);
        p2_new_elo = computeElo(p2_old_elo, p1_old_elo, 0.0);
    } else if (winner_id == player2_id) {
        p1_new_elo = computeElo(p1_old_elo, p2_old_elo, 0.0);
        p2_new_elo = computeElo(p2_old_elo, p1_old_elo, 1.0);
    }

    p1_change = p1_new_elo - p1_old_elo;
    p2_change = p2_new_elo - p2_old_elo;

    // Persist ELO changes
    if (db_) {
        db_->updateEloRating(player1_id, p1_new_elo);
        db_->updateEloRating(player2_id, p2_new_elo);
    }

    // Update cached ELO in PlayerManager and broadcast to all clients
    auto player_manager = server_->getPlayerManager();
    if (player_manager) {
        player_manager->updatePlayerElo(player1_id, p1_new_elo);
        player_manager->updatePlayerElo(player2_id, p2_new_elo);
    }

    // Save replay data for match history
    if (db_) {
        std::string end_reason;
        switch (reason) {
            case END_NORMAL: end_reason = "normal"; break;
            case END_RESIGN: end_reason = "resign"; break;
            case END_DISCONNECT: end_reason = "disconnect"; break;
            case END_TIMEOUT: end_reason = "timeout"; break;
            case END_DRAW_AGREED: end_reason = "draw"; break;
            default: end_reason = "unknown"; break;
        }

        db_->saveReplayData(match_id,
                           p1_old_elo, p2_old_elo,
                           p1_new_elo, p2_new_elo,
                           total_moves,
                           duration,
                           end_reason);
    }

    msg1.elo_change = p1_change;
    msg1.new_elo = p1_new_elo;

    if (winner_id == 0) {
        msg1.result = RESULT_DRAW;
    } else if (winner_id == player1_id) {
        msg1.result = RESULT_WIN;
    } else {
        msg1.result = RESULT_LOSS;
    }

    MessageHeader header;
    memset(&header, 0, sizeof(header));
    header.type = MessageType::MATCH_END;
    header.length = sizeof(msg1);
    header.timestamp = time(nullptr);

    if (player_manager) {
        ClientConnection* p1_conn = player_manager->getClientConnection(player1_id);
        if (p1_conn) {
            server_->sendToClient(p1_conn->getSocketFd(), header, &msg1, sizeof(msg1));
        }

        // Send to player 2 with opposite result
        MatchEndMessage msg2 = msg1;
        if (winner_id == 0) {
            msg2.result = RESULT_DRAW;
        } else if (winner_id == player2_id) {
            msg2.result = RESULT_WIN;
        } else {
            msg2.result = RESULT_LOSS;
        }

        // For player 2, adjust ELO values
        msg2.elo_change = p2_change;
        msg2.new_elo = p2_new_elo;

        ClientConnection* p2_conn = player_manager->getClientConnection(player2_id);
        if (p2_conn) {
            server_->sendToClient(p2_conn->getSocketFd(), header, &msg2, sizeof(msg2));
        }
    }
}

void GameplayHandler::handlePlayerDisconnect(uint32_t disconnected_user_id) {
    std::cout << "[GAMEPLAY] Handling disconnect for user_id=" << disconnected_user_id << std::endl;

    // Find all matches involving this player
    std::vector<uint32_t> matches_to_end;

    {
        std::lock_guard<std::mutex> lock(matches_mutex_);
        for (const auto& pair : active_matches_) {
            auto match = pair.second;
            if (match && (match->player1_id == disconnected_user_id || match->player2_id == disconnected_user_id)) {
                matches_to_end.push_back(pair.first);
            }
        }
    }

    // End each match where this player was involved
    for (uint32_t match_id : matches_to_end) {
        auto match = getMatch(match_id);
        if (!match) continue;

        uint32_t player1_id = match->player1_id;
        uint32_t player2_id = match->player2_id;
        uint32_t opponent_id = (player1_id == disconnected_user_id) ? player2_id : player1_id;

        std::cout << "[GAMEPLAY] Match " << match_id << " - Player " << disconnected_user_id
                  << " disconnected, awarding win to player " << opponent_id << std::endl;

        // Opponent wins, disconnected player loses
        // Calculate match duration
        uint64_t duration = 0;
        if (match->start_time > 0) {
            duration = time(nullptr) - match->start_time;
        }

        // Update match in database
        if (db_) {
            db_->endMatch(match_id, opponent_id);
        }

        // Send match end to both players with disconnect reason
        sendMatchEnd(match_id, player1_id, player2_id, opponent_id,
                    END_DISCONNECT, "Opponent disconnected",
                    match->turn_number, duration);

        // Remove match from active matches
        removeMatch(match_id);

        // Update player statuses back to AVAILABLE
        auto player_manager = server_->getPlayerManager();
        if (player_manager) {
            player_manager->updatePlayerStatus(opponent_id, STATUS_AVAILABLE);
            // Disconnected player is already being removed by removeClient
        }
    }

    // Clean up pending rematches involving the disconnected player
    {
        std::lock_guard<std::mutex> lock(rematch_mutex_);
        for (auto it = pending_rematches_.begin(); it != pending_rematches_.end();) {
            if (it->second.first == disconnected_user_id ||
                it->second.second == disconnected_user_id) {
                std::cout << "[GAMEPLAY] Removing pending rematch for match_id="
                          << it->first << " due to disconnect" << std::endl;
                it = pending_rematches_.erase(it);
            } else {
                ++it;
            }
        }
    }
}
