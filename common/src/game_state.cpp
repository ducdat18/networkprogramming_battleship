#include "game_state.h"
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// Board implementation
Board::Board() : ships_count(0), ships_remaining(NUM_SHIPS) {
    // Initialize grid to empty
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            grid[i][j] = CELL_EMPTY;
        }
    }

    // Initialize ships
    memset(ships, 0, sizeof(ships));
}

Board::~Board() {
}

bool Board::isValidPlacement(ShipType type, Coordinate pos, Orientation orientation) {
    int length = getShipLength(type);

    // Check bounds
    if (pos.row < 0 || pos.col < 0) return false;
    if (orientation == HORIZONTAL && pos.col + length > BOARD_SIZE) return false;
    if (orientation == VERTICAL && pos.row + length > BOARD_SIZE) return false;

    // Check for overlaps
    for (int i = 0; i < length; i++) {
        int r = pos.row + (orientation == VERTICAL ? i : 0);
        int c = pos.col + (orientation == HORIZONTAL ? i : 0);
        if (grid[r][c] != CELL_EMPTY) {
            return false;
        }
    }

    return true;
}

bool Board::placeShip(ShipType type, Coordinate pos, Orientation orientation) {
    if (!isValidPlacement(type, pos, orientation)) {
        return false;
    }

    int length = getShipLength(type);

    // Place ship on grid
    for (int i = 0; i < length; i++) {
        int r = pos.row + (orientation == VERTICAL ? i : 0);
        int c = pos.col + (orientation == HORIZONTAL ? i : 0);
        grid[r][c] = CELL_SHIP;
    }

    // Add to ships array
    Ship& ship = ships[ships_count++];
    ship.type = type;
    ship.orientation = orientation;
    ship.position = pos;
    ship.length = length;
    ship.hits = 0;
    ship.is_sunk = false;

    return true;
}

bool Board::removeShip(ShipType type) {
    // Find and remove ship
    for (int i = 0; i < ships_count; i++) {
        if (ships[i].type == type) {
            // Remove from grid
            int length = ships[i].length;
            for (int j = 0; j < length; j++) {
                int r = ships[i].position.row + (ships[i].orientation == VERTICAL ? j : 0);
                int c = ships[i].position.col + (ships[i].orientation == HORIZONTAL ? j : 0);
                if (grid[r][c] == CELL_SHIP) {
                    grid[r][c] = CELL_EMPTY;
                }
            }

            // Remove from array
            for (int j = i; j < ships_count - 1; j++) {
                ships[j] = ships[j + 1];
            }
            ships_count--;
            return true;
        }
    }
    return false;
}

void Board::clearBoard() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            grid[i][j] = CELL_EMPTY;
        }
    }
    ships_count = 0;
    ships_remaining = NUM_SHIPS;

    // Clear all ship data
    memset(ships, 0, sizeof(ships));
}

ShotResult Board::processShot(Coordinate pos) {
    if (pos.row < 0 || pos.row >= BOARD_SIZE || pos.col < 0 || pos.col >= BOARD_SIZE) {
        return SHOT_MISS;
    }

    CellState cell = grid[pos.row][pos.col];

    if (cell == CELL_SHIP) {
        grid[pos.row][pos.col] = CELL_HIT;

        // Find which ship was hit
        for (int i = 0; i < ships_count; i++) {
            Ship& ship = ships[i];
            int length = ship.length;

            for (int j = 0; j < length; j++) {
                int r = ship.position.row + (ship.orientation == VERTICAL ? j : 0);
                int c = ship.position.col + (ship.orientation == HORIZONTAL ? j : 0);

                if (r == pos.row && c == pos.col) {
                    ship.hits++;
                    if (ship.hits >= ship.length) {
                        ship.is_sunk = true;
                        ships_remaining--;
                        return SHOT_SUNK;
                    }
                    return SHOT_HIT;
                }
            }
        }
        return SHOT_HIT;
    } else if (cell == CELL_EMPTY) {
        grid[pos.row][pos.col] = CELL_MISS;
        return SHOT_MISS;
    }

    return SHOT_MISS; // Already shot
}

bool Board::allShipsSunk() {
    return ships_remaining == 0;
}

CellState Board::getCell(int row, int col) const {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return CELL_EMPTY;
    }
    return grid[row][col];
}

void Board::setCell(int row, int col, CellState state) {
    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
        grid[row][col] = state;
    }
}

void Board::randomPlacement() {
    clearBoard();
    srand(time(NULL));

    ShipType types[] = {SHIP_CARRIER, SHIP_BATTLESHIP, SHIP_CRUISER, SHIP_SUBMARINE, SHIP_DESTROYER};

    for (int i = 0; i < NUM_SHIPS; i++) {
        bool placed = false;
        int attempts = 0;

        while (!placed && attempts < 1000) {
            Coordinate pos;
            pos.row = rand() % BOARD_SIZE;
            pos.col = rand() % BOARD_SIZE;
            Orientation orient = (rand() % 2 == 0) ? HORIZONTAL : VERTICAL;

            if (isValidPlacement(types[i], pos, orient)) {
                placeShip(types[i], pos, orient);
                placed = true;
            }
            attempts++;
        }
    }
}

// MatchState implementation
MatchState::MatchState()
    : current_turn_player_id(0), turn_number(0), turn_time_limit(20),
      turn_start_time(0), start_time(0), end_time(0), result(RESULT_DRAW),
      winner_id(0), is_active(false), is_paused(false) {
}

MatchState::~MatchState() {
}

void MatchState::startMatch() {
    is_active = true;
    start_time = time(NULL);
    turn_number = 1;
}

void MatchState::endMatch(uint32_t winner) {
    is_active = false;
    end_time = time(NULL);
    winner_id = winner;

    if (winner == player1_id) {
        result = RESULT_WIN;
    } else if (winner == player2_id) {
        result = RESULT_LOSS;
    } else {
        result = RESULT_DRAW;
    }
}

void MatchState::switchTurn() {
    current_turn_player_id = (current_turn_player_id == player1_id) ? player2_id : player1_id;
    turn_number++;
    turn_start_time = time(nullptr);  // Record turn start time
}

ShotResult MatchState::processMove(uint32_t player_id, Coordinate target) {
    if (!is_active || is_paused) {
        return SHOT_MISS;
    }

    if (player_id != current_turn_player_id) {
        return SHOT_MISS;
    }

    // Get target board
    Board* target_board = (player_id == player1_id) ? &player2_board : &player1_board;

    ShotResult result = target_board->processShot(target);

    // Create move record
    Move move;
    move.move_number = turn_number;
    move.player_id = player_id;
    move.target = target;
    move.result = result;
    move.timestamp = time(NULL);
    move.time_taken_ms = 0; // Would need to track turn start time

    addMoveToHistory(move);

    // Check for game end
    if (target_board->allShipsSunk()) {
        endMatch(player_id);
    } else {
        // Only switch turn on miss (optional rule)
        if (result == SHOT_MISS) {
            switchTurn();
        }
    }

    return result;
}

void MatchState::addMoveToHistory(const Move& move) {
    move_history.push_back(move);
}
