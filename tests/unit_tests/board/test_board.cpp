#include <gtest/gtest.h>
#include "game_state.h"

// Test fixture for Board tests
class BoardTest : public ::testing::Test {
protected:
    Board board;

    void SetUp() override {
        // Fresh board for each test
        board.clearBoard();
    }
};

// ============== SHIP PLACEMENT TESTS ==============

TEST_F(BoardTest, PlaceShip_ValidHorizontal) {
    Coordinate pos = {0, 0};
    bool result = board.placeShip(SHIP_CARRIER, pos, HORIZONTAL);

    EXPECT_TRUE(result);
    EXPECT_EQ(board.getCell(0, 0), CELL_SHIP);
    EXPECT_EQ(board.getCell(0, 4), CELL_SHIP);  // Carrier length = 5
    EXPECT_EQ(board.getCell(0, 5), CELL_EMPTY);
}

TEST_F(BoardTest, PlaceShip_ValidVertical) {
    Coordinate pos = {0, 0};
    bool result = board.placeShip(SHIP_BATTLESHIP, pos, VERTICAL);

    EXPECT_TRUE(result);
    EXPECT_EQ(board.getCell(0, 0), CELL_SHIP);
    EXPECT_EQ(board.getCell(3, 0), CELL_SHIP);  // Battleship length = 4
    EXPECT_EQ(board.getCell(4, 0), CELL_EMPTY);
}

TEST_F(BoardTest, PlaceShip_OutOfBounds_Horizontal) {
    Coordinate pos = {0, 8};  // Carrier length 5 would go beyond board
    bool result = board.placeShip(SHIP_CARRIER, pos, HORIZONTAL);

    EXPECT_FALSE(result);
    EXPECT_EQ(board.getCell(0, 8), CELL_EMPTY);
}

TEST_F(BoardTest, PlaceShip_OutOfBounds_Vertical) {
    Coordinate pos = {8, 0};  // Battleship length 4 would go beyond board
    bool result = board.placeShip(SHIP_BATTLESHIP, pos, VERTICAL);

    EXPECT_FALSE(result);
    EXPECT_EQ(board.getCell(8, 0), CELL_EMPTY);
}

TEST_F(BoardTest, PlaceShip_Collision) {
    Coordinate pos1 = {0, 0};
    Coordinate pos2 = {0, 2};  // Overlaps with first ship

    board.placeShip(SHIP_CARRIER, pos1, HORIZONTAL);
    bool result = board.placeShip(SHIP_BATTLESHIP, pos2, HORIZONTAL);

    EXPECT_FALSE(result);
}

TEST_F(BoardTest, PlaceShip_Adjacent_ShouldSucceed) {
    Coordinate pos1 = {0, 0};
    Coordinate pos2 = {1, 0};  // Adjacent but not overlapping

    board.placeShip(SHIP_CARRIER, pos1, HORIZONTAL);
    bool result = board.placeShip(SHIP_BATTLESHIP, pos2, HORIZONTAL);

    EXPECT_TRUE(result);
}

TEST_F(BoardTest, PlaceAllShips) {
    Coordinate positions[] = {
        {0, 0}, {2, 0}, {4, 0}, {6, 0}, {8, 0}
    };
    ShipType types[] = {
        SHIP_CARRIER, SHIP_BATTLESHIP, SHIP_CRUISER,
        SHIP_SUBMARINE, SHIP_DESTROYER
    };

    for (int i = 0; i < 5; i++) {
        bool result = board.placeShip(types[i], positions[i], HORIZONTAL);
        EXPECT_TRUE(result) << "Failed to place ship " << i;
    }
}

// ============== SHOT PROCESSING TESTS ==============

TEST_F(BoardTest, ProcessShot_Miss) {
    Coordinate pos = {0, 0};
    ShotResult result = board.processShot(pos);

    EXPECT_EQ(result, SHOT_MISS);
    EXPECT_EQ(board.getCell(0, 0), CELL_MISS);
}

TEST_F(BoardTest, ProcessShot_Hit) {
    // Place a ship first
    Coordinate ship_pos = {0, 0};
    board.placeShip(SHIP_DESTROYER, ship_pos, HORIZONTAL);  // length 2

    // Shoot at ship
    ShotResult result = board.processShot(ship_pos);

    EXPECT_EQ(result, SHOT_HIT);
    EXPECT_EQ(board.getCell(0, 0), CELL_HIT);
}

TEST_F(BoardTest, ProcessShot_Sunk) {
    // Place destroyer (length 2)
    Coordinate ship_pos = {0, 0};
    board.placeShip(SHIP_DESTROYER, ship_pos, HORIZONTAL);

    // Hit both cells
    board.processShot({0, 0});
    ShotResult result = board.processShot({0, 1});

    EXPECT_EQ(result, SHOT_SUNK);
    EXPECT_FALSE(board.allShipsSunk());  // Still have 4 ships
}

TEST_F(BoardTest, ProcessShot_OutOfBounds) {
    Coordinate invalid = {-1, 0};
    ShotResult result = board.processShot(invalid);

    EXPECT_EQ(result, SHOT_MISS);
}

TEST_F(BoardTest, ProcessShot_AlreadyShot) {
    Coordinate pos = {0, 0};

    // First shot
    board.processShot(pos);
    EXPECT_EQ(board.getCell(0, 0), CELL_MISS);

    // Second shot at same position
    ShotResult result = board.processShot(pos);
    EXPECT_EQ(result, SHOT_MISS);
}

TEST_F(BoardTest, AllShipsSunk_Complete) {
    // Place one destroyer (smallest ship)
    Coordinate pos = {0, 0};
    board.placeShip(SHIP_DESTROYER, pos, HORIZONTAL);
    board.clearBoard();  // Clear to place just one ship manually

    // Set cells manually for testing
    board.setCell(0, 0, CELL_SHIP);
    board.setCell(0, 1, CELL_SHIP);

    // This test shows a limitation - we need a proper way to test victory
    // For now, we'll test the logic in MatchState instead
}

// ============== REMOVE SHIP TESTS ==============

TEST_F(BoardTest, RemoveShip_Success) {
    Coordinate pos = {0, 0};
    board.placeShip(SHIP_CARRIER, pos, HORIZONTAL);

    bool result = board.removeShip(SHIP_CARRIER);

    EXPECT_TRUE(result);
    EXPECT_EQ(board.getCell(0, 0), CELL_EMPTY);
    EXPECT_EQ(board.getCell(0, 4), CELL_EMPTY);
}

TEST_F(BoardTest, RemoveShip_NotFound) {
    bool result = board.removeShip(SHIP_CARRIER);

    EXPECT_FALSE(result);
}

// ============== CLEAR BOARD TESTS ==============

TEST_F(BoardTest, ClearBoard) {
    // Place ships
    board.placeShip(SHIP_CARRIER, {0, 0}, HORIZONTAL);
    board.placeShip(SHIP_BATTLESHIP, {2, 0}, HORIZONTAL);

    // Clear
    board.clearBoard();

    // Verify all cells empty
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            EXPECT_EQ(board.getCell(i, j), CELL_EMPTY);
        }
    }
}

// ============== RANDOM PLACEMENT TESTS ==============

TEST_F(BoardTest, RandomPlacement_PlacesAllShips) {
    board.randomPlacement();

    // Count ships on board
    int ship_cells = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board.getCell(i, j) == CELL_SHIP) {
                ship_cells++;
            }
        }
    }

    // Total: Carrier(5) + Battleship(4) + Cruiser(3) + Submarine(3) + Destroyer(2) = 17
    EXPECT_EQ(ship_cells, 17);
}

// Main function
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
