#include <gtest/gtest.h>
#include "player_manager.h"
#include "server.h"
#include "client_connection.h"
#include <thread>
#include <chrono>

/**
 * Unit Tests for PlayerManager
 *
 * Tests:
 * - Player registration (add/remove)
 * - Status tracking and updates
 * - Player list retrieval
 * - Available players filtering
 * - Thread safety (concurrent operations)
 */

class PlayerManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Pass nullptr for unit tests (no broadcasting)
        player_manager = new PlayerManager(nullptr);
    }

    void TearDown() override {
        delete player_manager;
    }

    PlayerManager* player_manager;
};

// Test: Add player
TEST_F(PlayerManagerTest, AddPlayer) {
    player_manager->addPlayer(nullptr, 1, "user1", "Player One", 1000);

    EXPECT_TRUE(player_manager->isPlayerOnline(1));
    EXPECT_EQ(player_manager->getPlayerStatus(1), STATUS_AVAILABLE);
}

// Test: Add multiple players
TEST_F(PlayerManagerTest, AddMultiplePlayers) {
    player_manager->addPlayer(nullptr, 1, "user1", "Player One", 1000);
    player_manager->addPlayer(nullptr, 2, "user2", "Player Two", 1200);
    player_manager->addPlayer(nullptr, 3, "user3", "Player Three", 800);

    EXPECT_TRUE(player_manager->isPlayerOnline(1));
    EXPECT_TRUE(player_manager->isPlayerOnline(2));
    EXPECT_TRUE(player_manager->isPlayerOnline(3));

    auto players = player_manager->getOnlinePlayers();
    EXPECT_EQ(players.size(), 3);
}

// Test: Remove player
TEST_F(PlayerManagerTest, RemovePlayer) {
    player_manager->addPlayer(nullptr, 1, "user1", "Player One", 1000);
    EXPECT_TRUE(player_manager->isPlayerOnline(1));

    player_manager->removePlayer(1);
    EXPECT_FALSE(player_manager->isPlayerOnline(1));
    EXPECT_EQ(player_manager->getPlayerStatus(1), STATUS_OFFLINE);
}

// Test: Update player status
TEST_F(PlayerManagerTest, UpdatePlayerStatus) {
    player_manager->addPlayer(nullptr, 1, "user1", "Player One", 1000);
    EXPECT_EQ(player_manager->getPlayerStatus(1), STATUS_AVAILABLE);

    player_manager->updatePlayerStatus(1, STATUS_IN_GAME);
    EXPECT_EQ(player_manager->getPlayerStatus(1), STATUS_IN_GAME);

    player_manager->updatePlayerStatus(1, STATUS_BUSY);
    EXPECT_EQ(player_manager->getPlayerStatus(1), STATUS_BUSY);
}

// Test: Get player info
TEST_F(PlayerManagerTest, GetPlayerInfo) {
    player_manager->addPlayer(nullptr, 1, "user1", "Player One", 1000);

    PlayerInfo_Message info = player_manager->getPlayerInfo(1);
    EXPECT_EQ(info.user_id, 1);
    EXPECT_STREQ(info.username, "user1");
    EXPECT_STREQ(info.display_name, "Player One");
    EXPECT_EQ(info.elo_rating, 1000);
    EXPECT_EQ(info.status, STATUS_AVAILABLE);
}

// Test: Get non-existent player info
TEST_F(PlayerManagerTest, GetNonExistentPlayerInfo) {
    PlayerInfo_Message info = player_manager->getPlayerInfo(999);
    EXPECT_EQ(info.user_id, 0);
}

// Test: Get online players list
TEST_F(PlayerManagerTest, GetOnlinePlayers) {
    player_manager->addPlayer(nullptr, 1, "user1", "Player One", 1000);
    player_manager->addPlayer(nullptr, 2, "user2", "Player Two", 1200);
    player_manager->addPlayer(nullptr, 3, "user3", "Player Three", 800);

    auto players = player_manager->getOnlinePlayers();
    EXPECT_EQ(players.size(), 3);

    // Verify all players are in the list
    bool found1 = false, found2 = false, found3 = false;
    for (const auto& p : players) {
        if (p.user_id == 1) found1 = true;
        if (p.user_id == 2) found2 = true;
        if (p.user_id == 3) found3 = true;
    }
    EXPECT_TRUE(found1 && found2 && found3);
}

// Test: Get available players (filter by status)
TEST_F(PlayerManagerTest, GetAvailablePlayers) {
    player_manager->addPlayer(nullptr, 1, "user1", "Player One", 1000);
    player_manager->addPlayer(nullptr, 2, "user2", "Player Two", 1200);
    player_manager->addPlayer(nullptr, 3, "user3", "Player Three", 800);

    // Set player 2 to in_game
    player_manager->updatePlayerStatus(2, STATUS_IN_GAME);

    auto available = player_manager->getAvailablePlayers();
    EXPECT_EQ(available.size(), 2);

    // Verify only available players (1 and 3)
    for (const auto& p : available) {
        EXPECT_TRUE(p.user_id == 1 || p.user_id == 3);
        EXPECT_EQ(p.status, STATUS_AVAILABLE);
    }
}

// Test: Empty player list
TEST_F(PlayerManagerTest, EmptyPlayerList) {
    auto players = player_manager->getOnlinePlayers();
    EXPECT_EQ(players.size(), 0);

    auto available = player_manager->getAvailablePlayers();
    EXPECT_EQ(available.size(), 0);
}

// Test: Player status transitions
TEST_F(PlayerManagerTest, PlayerStatusTransitions) {
    player_manager->addPlayer(nullptr, 1, "user1", "Player One", 1000);

    // Available -> In Game -> Available
    EXPECT_EQ(player_manager->getPlayerStatus(1), STATUS_AVAILABLE);

    player_manager->updatePlayerStatus(1, STATUS_IN_GAME);
    EXPECT_EQ(player_manager->getPlayerStatus(1), STATUS_IN_GAME);

    player_manager->updatePlayerStatus(1, STATUS_AVAILABLE);
    EXPECT_EQ(player_manager->getPlayerStatus(1), STATUS_AVAILABLE);
}

// Test: Concurrent add/remove operations (thread safety)
TEST_F(PlayerManagerTest, ConcurrentAddRemove) {
    const int num_threads = 10;
    const int players_per_thread = 10;

    std::vector<std::thread> threads;

    // Add players concurrently
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, players_per_thread]() {
            for (int i = 0; i < players_per_thread; i++) {
                uint32_t user_id = t * players_per_thread + i + 1;
                std::string username = "user" + std::to_string(user_id);
                std::string display_name = "Player " + std::to_string(user_id);
                player_manager->addPlayer(nullptr, user_id, username, display_name, 1000);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    // Verify all players were added
    auto players = player_manager->getOnlinePlayers();
    EXPECT_EQ(players.size(), num_threads * players_per_thread);

    // Remove players concurrently
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, players_per_thread]() {
            for (int i = 0; i < players_per_thread; i++) {
                uint32_t user_id = t * players_per_thread + i + 1;
                player_manager->removePlayer(user_id);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify all players were removed
    players = player_manager->getOnlinePlayers();
    EXPECT_EQ(players.size(), 0);
}

// Test: Concurrent status updates (thread safety)
TEST_F(PlayerManagerTest, ConcurrentStatusUpdates) {
    const int num_players = 50;

    // Add players
    for (int i = 1; i <= num_players; i++) {
        std::string username = "user" + std::to_string(i);
        std::string display_name = "Player " + std::to_string(i);
        player_manager->addPlayer(nullptr, i, username, display_name, 1000);
    }

    // Update statuses concurrently
    std::vector<std::thread> threads;
    for (int i = 1; i <= num_players; i++) {
        threads.emplace_back([this, i]() {
            player_manager->updatePlayerStatus(i, STATUS_IN_GAME);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            player_manager->updatePlayerStatus(i, STATUS_AVAILABLE);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify all players are available
    auto available = player_manager->getAvailablePlayers();
    EXPECT_EQ(available.size(), num_players);
}

// Test: Get client connection
TEST_F(PlayerManagerTest, GetClientConnection) {
    ClientConnection* fake_client = reinterpret_cast<ClientConnection*>(0x1234);
    player_manager->addPlayer(fake_client, 1, "user1", "Player One", 1000);

    ClientConnection* retrieved = player_manager->getClientConnection(1);
    EXPECT_EQ(retrieved, fake_client);

    ClientConnection* non_existent = player_manager->getClientConnection(999);
    EXPECT_EQ(non_existent, nullptr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
