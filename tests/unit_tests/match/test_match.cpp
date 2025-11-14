#include <gtest/gtest.h>
#include "game_state.h"

// Test fixture for MatchState tests
class MatchStateTest : public ::testing::Test {
protected:
    MatchState match;

    void SetUp() override {
        // Setup basic match
        match.player1_id = 1;
        match.player2_id = 2;
        match.player1_name = "Player1";
        match.player2_name = "Player2";
    }

    // Helper: Place ships for both players
    void setupShips() {
        Coordinate pos = {0, 0};
        match.player1_board.placeShip(SHIP_DESTROYER, pos, HORIZONTAL);
        match.player2_board.placeShip(SHIP_DESTROYER, {5, 5}, HORIZONTAL);
    }
};

// ============== MATCH LIFECYCLE TESTS ==============

TEST_F(MatchStateTest, StartMatch) {
    match.startMatch();

    EXPECT_TRUE(match.is_active);
    EXPECT_FALSE(match.is_paused);
    EXPECT_EQ(match.turn_number, 1);
    EXPECT_GT(match.start_time, 0);
}

TEST_F(MatchStateTest, EndMatch_Player1Wins) {
    match.startMatch();
    match.endMatch(match.player1_id);

    EXPECT_FALSE(match.is_active);
    EXPECT_EQ(match.winner_id, match.player1_id);
    EXPECT_EQ(match.result, RESULT_WIN);
    EXPECT_GT(match.end_time, 0);
}

TEST_F(MatchStateTest, EndMatch_Player2Wins) {
    match.startMatch();
    match.endMatch(match.player2_id);

    EXPECT_FALSE(match.is_active);
    EXPECT_EQ(match.winner_id, match.player2_id);
    EXPECT_EQ(match.result, RESULT_LOSS);  // From player1's perspective
}

TEST_F(MatchStateTest, EndMatch_Draw) {
    match.startMatch();
    match.endMatch(0);  // No winner

    EXPECT_FALSE(match.is_active);
    EXPECT_EQ(match.winner_id, 0);
    EXPECT_EQ(match.result, RESULT_DRAW);
}

// ============== TURN MANAGEMENT TESTS ==============

TEST_F(MatchStateTest, SwitchTurn) {
    match.current_turn_player_id = match.player1_id;
    match.turn_number = 1;

    match.switchTurn();

    EXPECT_EQ(match.current_turn_player_id, match.player2_id);
    EXPECT_EQ(match.turn_number, 2);
}

TEST_F(MatchStateTest, SwitchTurn_BackAndForth) {
    match.current_turn_player_id = match.player1_id;

    match.switchTurn();
    EXPECT_EQ(match.current_turn_player_id, match.player2_id);

    match.switchTurn();
    EXPECT_EQ(match.current_turn_player_id, match.player1_id);
}

// ============== MOVE PROCESSING TESTS ==============

TEST_F(MatchStateTest, ProcessMove_NotActive) {
    // Match not started
    setupShips();
    match.current_turn_player_id = match.player1_id;

    Coordinate target = {0, 0};
    ShotResult result = match.processMove(match.player1_id, target);

    EXPECT_EQ(result, SHOT_MISS);  // Returns miss when not active
}

TEST_F(MatchStateTest, ProcessMove_WrongTurn) {
    setupShips();
    match.startMatch();
    match.current_turn_player_id = match.player1_id;

    Coordinate target = {0, 0};
    ShotResult result = match.processMove(match.player2_id, target);  // Wrong player

    EXPECT_EQ(result, SHOT_MISS);
}

TEST_F(MatchStateTest, ProcessMove_ValidMiss) {
    setupShips();
    match.startMatch();
    match.current_turn_player_id = match.player1_id;

    Coordinate target = {9, 9};  // Empty cell
    ShotResult result = match.processMove(match.player1_id, target);

    EXPECT_EQ(result, SHOT_MISS);
    EXPECT_EQ(match.current_turn_player_id, match.player2_id);  // Turn switched
    EXPECT_EQ(match.move_history.size(), 1);
}

TEST_F(MatchStateTest, ProcessMove_ValidHit) {
    setupShips();
    match.startMatch();
    match.current_turn_player_id = match.player1_id;

    Coordinate target = {5, 5};  // Where player2's destroyer is
    ShotResult result = match.processMove(match.player1_id, target);

    EXPECT_EQ(result, SHOT_HIT);
    EXPECT_EQ(match.current_turn_player_id, match.player1_id);  // Turn NOT switched
    EXPECT_EQ(match.move_history.size(), 1);
}

TEST_F(MatchStateTest, ProcessMove_SinkShip) {
    setupShips();
    match.startMatch();
    match.current_turn_player_id = match.player1_id;

    // Hit both cells of destroyer
    match.processMove(match.player1_id, {5, 5});
    ShotResult result = match.processMove(match.player1_id, {5, 6});

    EXPECT_EQ(result, SHOT_SUNK);
    EXPECT_EQ(match.move_history.size(), 2);
}

TEST_F(MatchStateTest, ProcessMove_WinGame) {
    // Place all 5 ships for both players (minimal setup)
    // Player 2's ships - all horizontal, stacked vertically
    match.player2_board.placeShip(SHIP_DESTROYER, {0, 0}, HORIZONTAL);   // 2 cells
    match.player2_board.placeShip(SHIP_SUBMARINE, {1, 0}, HORIZONTAL);   // 3 cells
    match.player2_board.placeShip(SHIP_CRUISER, {2, 0}, HORIZONTAL);     // 3 cells
    match.player2_board.placeShip(SHIP_BATTLESHIP, {3, 0}, HORIZONTAL);  // 4 cells
    match.player2_board.placeShip(SHIP_CARRIER, {4, 0}, HORIZONTAL);     // 5 cells

    // Player 1 also needs ships (won't be attacked in this test)
    match.player1_board.placeShip(SHIP_DESTROYER, {5, 5}, HORIZONTAL);
    match.player1_board.placeShip(SHIP_SUBMARINE, {6, 5}, HORIZONTAL);
    match.player1_board.placeShip(SHIP_CRUISER, {7, 5}, HORIZONTAL);
    match.player1_board.placeShip(SHIP_BATTLESHIP, {8, 5}, HORIZONTAL);
    match.player1_board.placeShip(SHIP_CARRIER, {9, 0}, HORIZONTAL);

    match.startMatch();
    match.current_turn_player_id = match.player1_id;

    // Sink all of player2's ships (total 17 shots)
    // Ship 1: Destroyer at (0,0)-(0,1)
    match.processMove(match.player1_id, {0, 0});
    match.processMove(match.player1_id, {0, 1});

    // Ship 2: Submarine at (1,0)-(1,2)
    match.processMove(match.player1_id, {1, 0});
    match.processMove(match.player1_id, {1, 1});
    match.processMove(match.player1_id, {1, 2});

    // Ship 3: Cruiser at (2,0)-(2,2)
    match.processMove(match.player1_id, {2, 0});
    match.processMove(match.player1_id, {2, 1});
    match.processMove(match.player1_id, {2, 2});

    // Ship 4: Battleship at (3,0)-(3,3)
    match.processMove(match.player1_id, {3, 0});
    match.processMove(match.player1_id, {3, 1});
    match.processMove(match.player1_id, {3, 2});
    match.processMove(match.player1_id, {3, 3});

    // Ship 5: Carrier at (4,0)-(4,4) - last ship
    match.processMove(match.player1_id, {4, 0});
    match.processMove(match.player1_id, {4, 1});
    match.processMove(match.player1_id, {4, 2});
    match.processMove(match.player1_id, {4, 3});
    ShotResult result = match.processMove(match.player1_id, {4, 4});

    EXPECT_EQ(result, SHOT_SUNK);
    EXPECT_FALSE(match.is_active);  // Match ended
    EXPECT_EQ(match.winner_id, match.player1_id);
}

TEST_F(MatchStateTest, ProcessMove_Paused) {
    setupShips();
    match.startMatch();
    match.is_paused = true;
    match.current_turn_player_id = match.player1_id;

    Coordinate target = {0, 0};
    ShotResult result = match.processMove(match.player1_id, target);

    EXPECT_EQ(result, SHOT_MISS);  // Can't move when paused
}

// ============== MOVE HISTORY TESTS ==============

TEST_F(MatchStateTest, MoveHistory_RecordsCorrectly) {
    setupShips();
    match.startMatch();
    match.current_turn_player_id = match.player1_id;

    match.processMove(match.player1_id, {0, 0});
    match.processMove(match.player2_id, {1, 1});

    EXPECT_EQ(match.move_history.size(), 2);
    EXPECT_EQ(match.move_history[0].player_id, match.player1_id);
    EXPECT_EQ(match.move_history[0].target.row, 0);
    EXPECT_EQ(match.move_history[0].target.col, 0);
    EXPECT_EQ(match.move_history[1].player_id, match.player2_id);
}

TEST_F(MatchStateTest, MoveHistory_Empty_Initially) {
    EXPECT_EQ(match.move_history.size(), 0);
}

// ============== INTEGRATION TESTS ==============

TEST_F(MatchStateTest, CompleteGameFlow) {
    // Setup complete game with all ships
    // Player 2's ships
    match.player2_board.placeShip(SHIP_DESTROYER, {0, 0}, HORIZONTAL);
    match.player2_board.placeShip(SHIP_SUBMARINE, {1, 0}, HORIZONTAL);
    match.player2_board.placeShip(SHIP_CRUISER, {2, 0}, HORIZONTAL);
    match.player2_board.placeShip(SHIP_BATTLESHIP, {3, 0}, HORIZONTAL);
    match.player2_board.placeShip(SHIP_CARRIER, {4, 0}, HORIZONTAL);

    // Player 1's ships
    match.player1_board.placeShip(SHIP_DESTROYER, {0, 5}, HORIZONTAL);
    match.player1_board.placeShip(SHIP_SUBMARINE, {1, 5}, HORIZONTAL);
    match.player1_board.placeShip(SHIP_CRUISER, {2, 5}, HORIZONTAL);
    match.player1_board.placeShip(SHIP_BATTLESHIP, {3, 5}, HORIZONTAL);
    match.player1_board.placeShip(SHIP_CARRIER, {4, 5}, HORIZONTAL);

    match.startMatch();
    match.current_turn_player_id = match.player1_id;

    int total_moves = 0;

    // Player1 sinks all of Player2's ships
    // Destroyer (2 hits)
    EXPECT_EQ(match.processMove(match.player1_id, {0, 0}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {0, 1}), SHOT_SUNK);
    total_moves++;

    // Submarine (3 hits)
    EXPECT_EQ(match.processMove(match.player1_id, {1, 0}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {1, 1}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {1, 2}), SHOT_SUNK);
    total_moves++;

    // Cruiser (3 hits)
    EXPECT_EQ(match.processMove(match.player1_id, {2, 0}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {2, 1}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {2, 2}), SHOT_SUNK);
    total_moves++;

    // Battleship (4 hits)
    EXPECT_EQ(match.processMove(match.player1_id, {3, 0}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {3, 1}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {3, 2}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {3, 3}), SHOT_SUNK);
    total_moves++;

    // Carrier (5 hits) - final ship
    EXPECT_EQ(match.processMove(match.player1_id, {4, 0}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {4, 1}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {4, 2}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {4, 3}), SHOT_HIT);
    total_moves++;
    EXPECT_EQ(match.processMove(match.player1_id, {4, 4}), SHOT_SUNK);
    total_moves++;

    // Game should be over
    EXPECT_FALSE(match.is_active);
    EXPECT_EQ(match.winner_id, match.player1_id);
    EXPECT_EQ(match.move_history.size(), total_moves);
}

// Main function
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
