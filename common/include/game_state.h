#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "protocol.h"
#include <vector>
#include <string>

#define BOARD_SIZE 10
#define NUM_SHIPS 5

// Cell state
enum CellState {
    CELL_EMPTY = 0,
    CELL_SHIP = 1,
    CELL_MISS = 2,
    CELL_HIT = 3,
    CELL_SUNK = 4  // For visually marking sunk ships
};

// Board class
class Board {
private:
    CellState grid[BOARD_SIZE][BOARD_SIZE];
    Ship ships[NUM_SHIPS];
    int ships_count;
    int ships_remaining;

public:
    Board();
    ~Board();

    // Ship placement
    bool placeShip(ShipType type, Coordinate pos, Orientation orientation);
    bool removeShip(ShipType type);
    void clearBoard();
    bool isValidPlacement(ShipType type, Coordinate pos, Orientation orientation);

    // Gameplay
    ShotResult processShot(Coordinate pos);
    bool allShipsSunk();

    // Getters
    CellState getCell(int row, int col) const;
    void setCell(int row, int col, CellState state);
    const Ship* getShips() const { return ships; }
    int getShipsRemaining() const { return ships_remaining; }

    // Board state
    void randomPlacement();
    std::string serialize() const;
    bool deserialize(const std::string& data);
};

// Move structure
struct Move {
    int move_number;
    uint32_t player_id;
    Coordinate target;
    ShotResult result;
    ShipType ship_sunk;
    uint64_t timestamp;
    uint32_t time_taken_ms;
};

// Match state
class MatchState {
public:
    std::string match_id;
    uint32_t player1_id;
    uint32_t player2_id;
    std::string player1_name;
    std::string player2_name;

    int player1_elo_before;
    int player1_elo_after;
    int player2_elo_before;
    int player2_elo_after;

    Board player1_board;
    Board player2_board;

    uint32_t current_turn_player_id;
    int turn_number;
    int turn_time_limit;  // seconds
    uint64_t turn_start_time;  // Timestamp when current turn started

    std::vector<Move> move_history;

    uint64_t start_time;
    uint64_t end_time;

    GameResult result;
    uint32_t winner_id;

    bool is_active;
    bool is_paused;

    MatchState();
    ~MatchState();

    // Match control
    void startMatch();
    void endMatch(uint32_t winner_id);
    void switchTurn();

    // Move processing
    ShotResult processMove(uint32_t player_id, Coordinate target);
    void addMoveToHistory(const Move& move);

    // Serialization
    std::string serialize() const;
    bool deserialize(const std::string& data);
};

// Player info
struct PlayerInfo {
    uint32_t user_id;
    std::string username;
    std::string display_name;
    int elo_rating;
    int highest_elo;

    int total_games;
    int wins;
    int losses;
    int draws;

    PlayerStatus status;
    uint64_t last_seen;

    PlayerInfo() : user_id(0), elo_rating(1000), highest_elo(1000),
                   total_games(0), wins(0), losses(0), draws(0),
                   status(STATUS_OFFLINE), last_seen(0) {}
};

#endif // GAME_STATE_H
