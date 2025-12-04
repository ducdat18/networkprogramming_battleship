/**
 * Integration test for auto-login feature
 * Tests the complete flow: register -> login -> save session -> reload session
 */

#include <gtest/gtest.h>
#include "client_network.h"
#include "session_storage.h"
#include "protocol.h"
#include "config.h"
#include <thread>
#include <chrono>

class AutoLoginTest : public ::testing::Test {
protected:
    ClientNetwork* client;
    std::string test_username;

    void SetUp() override {
        client = new ClientNetwork();

        // Generate unique username for this test
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();
        test_username = "auto_login_test_" + std::to_string(timestamp);

        // Clear any existing session
        SessionStorage::clearSession();
    }

    void TearDown() override {
        if (client) {
            client->disconnect();
            delete client;
        }

        // Clean up session after test
        SessionStorage::clearSession();
    }

    bool waitForResponse(int timeout_ms = 5000) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start).count() < timeout_ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }
};

// Test: Complete auto-login flow
TEST_F(AutoLoginTest, CompleteAutoLoginFlow) {
    // Step 1: Connect to server
    bool conn_success = false;
    client->connect("127.0.0.1", SERVER_PORT, [&](bool success, const std::string& error) {
        conn_success = success;
        if (!success) {
            std::cout << "[ERROR] Connection failed: " << error << std::endl;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(conn_success) << "Failed to connect to server at 127.0.0.1:" << SERVER_PORT;

    // Step 2: Register a new user
    std::string password = "test_password_123";
    std::string display_name = "Auto Login Test User";

    bool register_success = false;
    uint32_t registered_user_id = 0;

    client->registerUser(test_username, password, display_name,
        [&](bool success, uint32_t user_id, const std::string& error) {
            register_success = success;
            registered_user_id = user_id;
            if (!success) {
                std::cout << "[ERROR] Register failed: " << error << std::endl;
            }
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(register_success);

    // Step 3: Login
    bool login_success = false;
    uint32_t user_id = 0;
    std::string session_token;
    int32_t elo_rating = 0;

    client->loginUser(test_username, password,
        [&](bool success, uint32_t uid, const std::string& dname,
            int32_t elo, const std::string& token, const std::string& error) {
            login_success = success;
            user_id = uid;
            display_name = dname;
            elo_rating = elo;
            session_token = token;
            if (!success) {
                std::cout << "[ERROR] Login failed: " << error << std::endl;
            }
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(login_success);
    ASSERT_GT(user_id, 0u);
    ASSERT_FALSE(session_token.empty());

    // Step 5: Save session
    bool saved = SessionStorage::saveSession(user_id, session_token,
                                             test_username, display_name, elo_rating);
    ASSERT_TRUE(saved) << "Failed to save session";

    // Step 6: Verify session file exists
    EXPECT_TRUE(SessionStorage::hasStoredSession());

    // Step 7: Load session (simulating app restart)
    uint32_t loaded_user_id;
    std::string loaded_token;
    std::string loaded_username;
    std::string loaded_display_name;
    int32_t loaded_elo;

    bool loaded = SessionStorage::loadSession(loaded_user_id, loaded_token,
                                              loaded_username, loaded_display_name,
                                              loaded_elo);
    ASSERT_TRUE(loaded) << "Failed to load session";

    // Step 8: Verify loaded data matches saved data
    EXPECT_EQ(loaded_user_id, user_id);
    EXPECT_EQ(loaded_token, session_token);
    EXPECT_EQ(loaded_username, test_username);
    EXPECT_EQ(loaded_display_name, display_name);
    EXPECT_EQ(loaded_elo, elo_rating);

    std::cout << "[TEST] Auto-login flow successful!" << std::endl;
    std::cout << "  User ID: " << loaded_user_id << std::endl;
    std::cout << "  Username: " << loaded_username << std::endl;
    std::cout << "  Display Name: " << loaded_display_name << std::endl;
    std::cout << "  ELO: " << loaded_elo << std::endl;
}

// Test: Session persistence across multiple saves
TEST_F(AutoLoginTest, SessionOverwrite) {
    // Save first session
    SessionStorage::saveSession(1, "token1", "user1", "User One", 1000);

    // Save second session (should overwrite)
    SessionStorage::saveSession(2, "token2", "user2", "User Two", 2000);

    // Load session
    uint32_t user_id;
    std::string token, username, display_name;
    int32_t elo;

    SessionStorage::loadSession(user_id, token, username, display_name, elo);

    // Should have second session
    EXPECT_EQ(user_id, 2u);
    EXPECT_EQ(token, "token2");
    EXPECT_EQ(username, "user2");
}

// Test: Clear session removes file
TEST_F(AutoLoginTest, ClearSessionRemovesFile) {
    // Save session
    SessionStorage::saveSession(1, "token", "user", "User", 1000);
    EXPECT_TRUE(SessionStorage::hasStoredSession());

    // Clear session
    SessionStorage::clearSession();

    // Should no longer exist
    EXPECT_FALSE(SessionStorage::hasStoredSession());

    // Load should fail
    uint32_t user_id;
    std::string token, username, display_name;
    int32_t elo;

    bool loaded = SessionStorage::loadSession(user_id, token, username, display_name, elo);
    EXPECT_FALSE(loaded);
}

// Test: Auto-login with real server authentication
TEST_F(AutoLoginTest, RealServerAutoLogin) {
    // Step 1: Connect to server
    bool conn_success = false;
    client->connect("127.0.0.1", SERVER_PORT, [&](bool success, const std::string& error) {
        conn_success = success;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(conn_success) << "Failed to connect to server. Make sure server is running on port " << SERVER_PORT;

    // Step 2: Register
    std::string password = "secure_password_456";
    std::string display_name = "Real Auth Test";

    bool register_success = false;
    client->registerUser(test_username, password, display_name,
        [&](bool success, uint32_t user_id, const std::string& error) {
            register_success = success;
        });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(register_success);

    // Step 3: Logout to test fresh login
    client->disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Step 4: Reconnect and login
    conn_success = false;
    client->connect("127.0.0.1", SERVER_PORT, [&](bool success, const std::string& error) {
        conn_success = success;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(conn_success);

    bool login_success = false;
    client->loginUser(test_username, password,
        [&](bool success, uint32_t uid, const std::string& dname,
            int32_t elo, const std::string& token, const std::string& error) {
            login_success = success;
        });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(login_success);

    std::cout << "[TEST] Real server auto-login test completed" << std::endl;
}

// Test: Session data integrity with special characters
TEST_F(AutoLoginTest, SessionDataIntegrity) {
    std::string special_token = "token_!@#$%^&*()_+-={}[]|:;<>?,./";
    std::string special_username = "user.name-123_test";
    std::string special_display = "Display Name (Special) [Test]";

    SessionStorage::saveSession(999, special_token, special_username, special_display, 2500);

    uint32_t user_id;
    std::string token, username, display_name;
    int32_t elo;

    SessionStorage::loadSession(user_id, token, username, display_name, elo);

    EXPECT_EQ(user_id, 999u);
    EXPECT_EQ(token, special_token);
    EXPECT_EQ(username, special_username);
    EXPECT_EQ(display_name, special_display);
    EXPECT_EQ(elo, 2500);
}

// Test: Multiple rapid save/load cycles
TEST_F(AutoLoginTest, RapidSaveLoadCycles) {
    for (int i = 0; i < 10; i++) {
        // Save session
        SessionStorage::saveSession(i, "token_" + std::to_string(i),
                                    "user_" + std::to_string(i),
                                    "User " + std::to_string(i),
                                    1000 + i * 100);

        // Load immediately
        uint32_t user_id;
        std::string token, username, display_name;
        int32_t elo;

        bool loaded = SessionStorage::loadSession(user_id, token, username, display_name, elo);
        ASSERT_TRUE(loaded);

        EXPECT_EQ(user_id, static_cast<uint32_t>(i));
        EXPECT_EQ(token, "token_" + std::to_string(i));
        EXPECT_EQ(elo, 1000 + i * 100);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════╗\n";
    std::cout << "║   AUTO-LOGIN INTEGRATION TESTS             ║\n";
    std::cout << "╠════════════════════════════════════════════╣\n";
    std::cout << "║  Requirements:                             ║\n";
    std::cout << "║  - Server running on localhost:" << SERVER_PORT << "        ║\n";
    std::cout << "║  - Database initialized                    ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n";
    std::cout << "\n";

    return RUN_ALL_TESTS();
}
