#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "challenge_manager.h"
#include "player_manager.h"
#include "server.h"
#include "database.h"
#include "client_connection.h"
#include <memory>

// Mock ClientConnection for testing
class MockClientConnection : public ClientConnection {
public:
    MockClientConnection(int fd) : ClientConnection(fd) {
    }

    void setupMockAuth(uint32_t user_id) {
        char token[64] = "test_token";
        setAuthenticated(user_id, std::string(token));
    }
};

class ChallengeManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp database with unique name per test
        std::string db_path = "/tmp/test_challenge_" + std::to_string(time(nullptr)) + "_" + std::to_string(rand()) + ".db";
        db_ = new DatabaseManager(db_path);
        ASSERT_TRUE(db_->isOpen()) << "Failed to open test database";

        // Create server with database
        server_ = new Server(9998);  // Use different port for tests

        // Get managers from server
        player_manager_ = server_->getPlayerManager();
        challenge_manager_ = server_->getChallengeManager();

        ASSERT_NE(player_manager_, nullptr);
        ASSERT_NE(challenge_manager_, nullptr);

        // Create mock clients
        client1_ = std::make_shared<MockClientConnection>(100);
        client2_ = std::make_shared<MockClientConnection>(101);
        client3_ = std::make_shared<MockClientConnection>(102);

        // Setup test users with unique names
        std::string suffix = std::to_string(rand());
        user1_id_ = db_->createUser("user1_" + suffix, "hash1", "User One");
        user2_id_ = db_->createUser("user2_" + suffix, "hash2", "User Two");
        user3_id_ = db_->createUser("user3_" + suffix, "hash3", "User Three");

        ASSERT_GT(user1_id_, 0u);
        ASSERT_GT(user2_id_, 0u);
        ASSERT_GT(user3_id_, 0u);

        client1_->setupMockAuth(user1_id_);
        client2_->setupMockAuth(user2_id_);
        client3_->setupMockAuth(user3_id_);

        // Add players to PlayerManager (cast to base class)
        player_manager_->addPlayer(static_cast<ClientConnection*>(client1_.get()), user1_id_, "user1", "User One", 1000);
        player_manager_->addPlayer(static_cast<ClientConnection*>(client2_.get()), user2_id_, "user2", "User Two", 1200);
        player_manager_->addPlayer(static_cast<ClientConnection*>(client3_.get()), user3_id_, "user3", "User Three", 1100);
    }

    void TearDown() override {
        delete server_;
        delete db_;
    }

    DatabaseManager* db_;
    Server* server_;
    PlayerManager* player_manager_;
    ChallengeManager* challenge_manager_;

    std::shared_ptr<MockClientConnection> client1_;
    std::shared_ptr<MockClientConnection> client2_;
    std::shared_ptr<MockClientConnection> client3_;

    uint32_t user1_id_;
    uint32_t user2_id_;
    uint32_t user3_id_;
};

// Test: Send valid challenge
TEST_F(ChallengeManagerTest, SendChallenge_Valid) {
    ChallengeRequest request;
    request.target_user_id = user2_id_;
    request.time_limit = 60;
    request.random_placement = false;

    bool result = challenge_manager_->sendChallenge(user1_id_, request);
    EXPECT_TRUE(result);
}

// Test: Cannot challenge self
TEST_F(ChallengeManagerTest, SendChallenge_CannotChallengeSelf) {
    ChallengeRequest request;
    request.target_user_id = user1_id_;  // Same as challenger
    request.time_limit = 60;
    request.random_placement = false;

    bool result = challenge_manager_->sendChallenge(user1_id_, request);
    EXPECT_FALSE(result);
}

// Test: Cannot challenge offline player
TEST_F(ChallengeManagerTest, SendChallenge_TargetOffline) {
    // Remove user2 from online players
    player_manager_->removePlayer(user2_id_);

    ChallengeRequest request;
    request.target_user_id = user2_id_;
    request.time_limit = 60;
    request.random_placement = false;

    bool result = challenge_manager_->sendChallenge(user1_id_, request);
    EXPECT_FALSE(result);
}

// Test: Cannot challenge player who is in game
TEST_F(ChallengeManagerTest, SendChallenge_TargetInGame) {
    // Set user2 to IN_GAME status
    player_manager_->updatePlayerStatus(user2_id_, STATUS_IN_GAME);

    ChallengeRequest request;
    request.target_user_id = user2_id_;
    request.time_limit = 60;
    request.random_placement = false;

    bool result = challenge_manager_->sendChallenge(user1_id_, request);
    EXPECT_FALSE(result);
}

// Test: Accept challenge creates match
TEST_F(ChallengeManagerTest, AcceptChallenge_CreatesMatch) {
    // Send challenge
    ChallengeRequest request;
    request.target_user_id = user2_id_;
    request.time_limit = 60;
    request.random_placement = false;

    bool sent = challenge_manager_->sendChallenge(user1_id_, request);
    ASSERT_TRUE(sent);

    // Accept challenge
    ChallengeResponse response;
    response.challenge_id = 1;  // First challenge has ID 1
    response.accepted = true;

    bool accepted = challenge_manager_->respondToChallenge(user2_id_, response);
    EXPECT_TRUE(accepted);

    // Check that both players are now IN_GAME
    EXPECT_EQ(player_manager_->getPlayerStatus(user1_id_), STATUS_IN_GAME);
    EXPECT_EQ(player_manager_->getPlayerStatus(user2_id_), STATUS_IN_GAME);
}

// Test: Decline challenge
TEST_F(ChallengeManagerTest, DeclineChallenge) {
    // Send challenge
    ChallengeRequest request;
    request.target_user_id = user2_id_;
    request.time_limit = 60;
    request.random_placement = false;

    bool sent = challenge_manager_->sendChallenge(user1_id_, request);
    ASSERT_TRUE(sent);

    // Decline challenge
    ChallengeResponse response;
    response.challenge_id = 1;
    response.accepted = false;

    bool declined = challenge_manager_->respondToChallenge(user2_id_, response);
    EXPECT_TRUE(declined);

    // Check that both players are still AVAILABLE
    EXPECT_EQ(player_manager_->getPlayerStatus(user1_id_), STATUS_AVAILABLE);
    EXPECT_EQ(player_manager_->getPlayerStatus(user2_id_), STATUS_AVAILABLE);
}

// Test: Cannot respond to non-existent challenge
TEST_F(ChallengeManagerTest, RespondToNonexistentChallenge) {
    ChallengeResponse response;
    response.challenge_id = 999;  // Does not exist
    response.accepted = true;

    bool result = challenge_manager_->respondToChallenge(user2_id_, response);
    EXPECT_FALSE(result);
}

// Test: Wrong user cannot respond to challenge
TEST_F(ChallengeManagerTest, WrongUserCannotRespond) {
    // User1 challenges User2
    ChallengeRequest request;
    request.target_user_id = user2_id_;
    request.time_limit = 60;
    request.random_placement = false;

    bool sent = challenge_manager_->sendChallenge(user1_id_, request);
    ASSERT_TRUE(sent);

    // User3 tries to respond (but challenge is for User2)
    ChallengeResponse response;
    response.challenge_id = 1;
    response.accepted = true;

    bool result = challenge_manager_->respondToChallenge(user3_id_, response);
    EXPECT_FALSE(result);
}

// Test: Multiple pending challenges
TEST_F(ChallengeManagerTest, MultiplePendingChallenges) {
    // User1 challenges User2
    ChallengeRequest request1;
    request1.target_user_id = user2_id_;
    request1.time_limit = 60;
    request1.random_placement = false;

    bool sent1 = challenge_manager_->sendChallenge(user1_id_, request1);
    EXPECT_TRUE(sent1);

    // User3 challenges User2
    ChallengeRequest request2;
    request2.target_user_id = user2_id_;
    request2.time_limit = 60;
    request2.random_placement = false;

    bool sent2 = challenge_manager_->sendChallenge(user3_id_, request2);
    EXPECT_TRUE(sent2);

    // User2 accepts first challenge
    ChallengeResponse response;
    response.challenge_id = 1;
    response.accepted = true;

    bool accepted = challenge_manager_->respondToChallenge(user2_id_, response);
    EXPECT_TRUE(accepted);
}

// Test: Remove player challenges on disconnect
TEST_F(ChallengeManagerTest, RemovePlayerChallenges) {
    // User1 challenges User2
    ChallengeRequest request;
    request.target_user_id = user2_id_;
    request.time_limit = 60;
    request.random_placement = false;

    bool sent = challenge_manager_->sendChallenge(user1_id_, request);
    ASSERT_TRUE(sent);

    // User1 disconnects
    challenge_manager_->removePlayerChallenges(user1_id_);

    // User2 tries to accept (should fail because challenge was removed)
    ChallengeResponse response;
    response.challenge_id = 1;
    response.accepted = true;

    bool result = challenge_manager_->respondToChallenge(user2_id_, response);
    EXPECT_FALSE(result);
}

// Test: Cancel challenge
TEST_F(ChallengeManagerTest, CancelChallenge) {
    // Send challenge
    ChallengeRequest request;
    request.target_user_id = user2_id_;
    request.time_limit = 60;
    request.random_placement = false;

    bool sent = challenge_manager_->sendChallenge(user1_id_, request);
    ASSERT_TRUE(sent);

    // Cancel challenge
    challenge_manager_->cancelChallenge(1);

    // Try to accept (should fail)
    ChallengeResponse response;
    response.challenge_id = 1;
    response.accepted = true;

    bool result = challenge_manager_->respondToChallenge(user2_id_, response);
    EXPECT_FALSE(result);
}

// Test: Challenge timeout (Note: This test would take 60+ seconds in real time)
// We'll skip this in unit tests and cover it in integration tests

int main(int argc, char **argv) {
    srand(time(nullptr));
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
