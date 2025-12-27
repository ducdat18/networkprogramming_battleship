/**
 * Unit tests for SessionStorage
 * Tests session persistence and retrieval
 */

#include <gtest/gtest.h>
#include "session_storage.h"
#include <unistd.h>
#include <fstream>

class SessionStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean any existing session before each test
        SessionStorage::clearSession();
    }

    void TearDown() override {
        // Clean up after each test
        SessionStorage::clearSession();
    }
};

// Test: Save and load session successfully
TEST_F(SessionStorageTest, SaveAndLoadSession) {
    uint32_t user_id = 123;
    std::string session_token = "test_token_abc123";
    std::string username = "testuser";
    std::string display_name = "Test User";
    int32_t elo_rating = 1500;

    // Save session
    bool saved = SessionStorage::saveSession(user_id, session_token, username, display_name, elo_rating);
    ASSERT_TRUE(saved);

    // Load session
    uint32_t loaded_user_id;
    std::string loaded_token;
    std::string loaded_username;
    std::string loaded_display_name;
    int32_t loaded_elo;

    bool loaded = SessionStorage::loadSession(loaded_user_id, loaded_token, loaded_username, loaded_display_name, loaded_elo);
    ASSERT_TRUE(loaded);

    // Verify loaded data matches saved data
    EXPECT_EQ(loaded_user_id, user_id);
    EXPECT_EQ(loaded_token, session_token);
    EXPECT_EQ(loaded_username, username);
    EXPECT_EQ(loaded_display_name, display_name);
    EXPECT_EQ(loaded_elo, elo_rating);
}

// Test: hasStoredSession returns correct status
TEST_F(SessionStorageTest, HasStoredSession) {
    // Initially no session
    EXPECT_FALSE(SessionStorage::hasStoredSession());

    // Save a session
    SessionStorage::saveSession(1, "token", "user", "User", 1000);

    // Now should have session
    EXPECT_TRUE(SessionStorage::hasStoredSession());
}

// Test: Clear session removes file
TEST_F(SessionStorageTest, ClearSession) {
    // Save a session
    SessionStorage::saveSession(1, "token", "user", "User", 1000);
    EXPECT_TRUE(SessionStorage::hasStoredSession());

    // Clear session
    SessionStorage::clearSession();

    // Should no longer have session
    EXPECT_FALSE(SessionStorage::hasStoredSession());
}

// Test: Load fails when no session exists
TEST_F(SessionStorageTest, LoadNonexistentSession) {
    uint32_t user_id;
    std::string token, username, display_name;
    int32_t elo;

    bool loaded = SessionStorage::loadSession(user_id, token, username, display_name, elo);
    EXPECT_FALSE(loaded);
}

// Test: Session with special characters
TEST_F(SessionStorageTest, SessionWithSpecialCharacters) {
    uint32_t user_id = 999;
    std::string session_token = "token_with_$pecial_ch@rs!";
    std::string username = "user.name-123";
    std::string display_name = "User Name (Special)";
    int32_t elo_rating = 2000;

    // Save session
    bool saved = SessionStorage::saveSession(user_id, session_token, username, display_name, elo_rating);
    ASSERT_TRUE(saved);

    // Load session
    uint32_t loaded_user_id;
    std::string loaded_token;
    std::string loaded_username;
    std::string loaded_display_name;
    int32_t loaded_elo;

    bool loaded = SessionStorage::loadSession(loaded_user_id, loaded_token, loaded_username, loaded_display_name, loaded_elo);
    ASSERT_TRUE(loaded);

    // Verify loaded data
    EXPECT_EQ(loaded_user_id, user_id);
    EXPECT_EQ(loaded_token, session_token);
    EXPECT_EQ(loaded_username, username);
    EXPECT_EQ(loaded_display_name, display_name);
    EXPECT_EQ(loaded_elo, elo_rating);
}

// Test: Overwrite existing session
TEST_F(SessionStorageTest, OverwriteSession) {
    // Save first session
    SessionStorage::saveSession(1, "token1", "user1", "User One", 1000);

    // Save second session (should overwrite)
    SessionStorage::saveSession(2, "token2", "user2", "User Two", 2000);

    // Load session
    uint32_t user_id;
    std::string token, username, display_name;
    int32_t elo;

    SessionStorage::loadSession(user_id, token, username, display_name, elo);

    // Should have second session data
    EXPECT_EQ(user_id, 2u);
    EXPECT_EQ(token, "token2");
    EXPECT_EQ(username, "user2");
    EXPECT_EQ(display_name, "User Two");
    EXPECT_EQ(elo, 2000);
}

// Test: Session with zero ELO
TEST_F(SessionStorageTest, SessionWithZeroELO) {
    SessionStorage::saveSession(1, "token", "user", "User", 0);

    uint32_t user_id;
    std::string token, username, display_name;
    int32_t elo;

    SessionStorage::loadSession(user_id, token, username, display_name, elo);

    EXPECT_EQ(elo, 0);
}

// Test: Session with negative ELO
TEST_F(SessionStorageTest, SessionWithNegativeELO) {
    SessionStorage::saveSession(1, "token", "user", "User", -100);

    uint32_t user_id;
    std::string token, username, display_name;
    int32_t elo;

    SessionStorage::loadSession(user_id, token, username, display_name, elo);

    EXPECT_EQ(elo, -100);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
