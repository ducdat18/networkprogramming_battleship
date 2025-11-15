/**
 * End-to-End Client Authentication Integration Test
 * Tests complete authentication flow: Connect → Register → Login → Logout
 *
 * NOTE: These tests are DISABLED by default.
 * To run: Use run_integration_tests.sh script
 */

#include <gtest/gtest.h>
#include "client_network.h"
#include "message_serialization.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace MessageSerialization;

const char* SERVER_HOST = "127.0.0.1";
const int SERVER_PORT = 8888;

class E2EClientAuthTest : public ::testing::Test {
protected:
    void SetUp() override {
        client = new ClientNetwork();

        // Connect to server
        std::atomic<bool> connected{false};
        client->connect(SERVER_HOST, SERVER_PORT, [&](bool success, const std::string& error) {
            connected = success;
            if (!success) {
                std::cerr << "Connection failed: " << error << std::endl;
            }
        });

        // Wait for connection
        int retries = 20;
        while (!connected && retries-- > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        ASSERT_TRUE(connected) << "Failed to connect to server";
    }

    void TearDown() override {
        if (client) {
            client->disconnect();
            delete client;
        }
    }

    ClientNetwork* client;
};

// ==================== Complete Flow Tests ====================

TEST_F(E2EClientAuthTest, DISABLED_CompleteAuthFlow_RegisterLoginLogout) {
    std::atomic<bool> done{false};
    std::atomic<int> step{0};  // Track progress
    uint32_t user_id = 0;
    std::string username = "e2e_test_user_" + std::to_string(time(nullptr));

    // Step 1: Register
    std::cout << "[TEST] Step 1: Registering user " << username << std::endl;
    client->registerUser(username, "password123", "E2E Test User",
        [&](bool success, uint32_t uid, const std::string& error) {
            EXPECT_TRUE(success) << "Registration failed: " << error;
            if (success) {
                user_id = uid;
                std::cout << "[TEST] Registered with user_id=" << uid << std::endl;
                step = 1;
            } else {
                done = true;  // Fail fast
            }
        });

    // Wait for registration
    while (step < 1 && !done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_EQ(step, 1);

    // Give server a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Step 2: Login
    std::cout << "[TEST] Step 2: Logging in as " << username << std::endl;
    client->loginUser(username, "password123",
        [&](bool success, uint32_t uid, const std::string& display_name,
            int32_t elo_rating, const std::string& session_token, const std::string& error) {
            EXPECT_TRUE(success) << "Login failed: " << error;
            if (success) {
                EXPECT_EQ(uid, user_id);
                EXPECT_EQ(display_name, "E2E Test User");
                EXPECT_EQ(elo_rating, 1000);  // Default ELO
                EXPECT_GT(session_token.length(), 0);
                std::cout << "[TEST] Logged in successfully, ELO=" << elo_rating << std::endl;
                step = 2;
            } else {
                done = true;
            }
        });

    // Wait for login
    while (step < 2 && !done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_EQ(step, 2);
    EXPECT_TRUE(client->isAuthenticated());
    EXPECT_EQ(client->getUserId(), user_id);

    // Give server a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Step 3: Logout
    std::cout << "[TEST] Step 3: Logging out" << std::endl;
    client->logoutUser([&](bool success) {
        EXPECT_TRUE(success);
        if (success) {
            std::cout << "[TEST] Logged out successfully" << std::endl;
            step = 3;
        }
        done = true;
    });

    // Wait for logout
    while (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ASSERT_EQ(step, 3);
}

TEST_F(E2EClientAuthTest, DISABLED_Login_WithWrongPassword) {
    std::atomic<bool> done{false};
    bool login_success = true;  // Will be set to false
    std::string error_msg;

    // First register a user
    std::string username = "wrong_pwd_test_" + std::to_string(time(nullptr));
    client->registerUser(username, "correct_password", "Test User",
        [](bool, uint32_t, const std::string&) {});

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Try to login with wrong password
    client->loginUser(username, "wrong_password",
        [&](bool success, uint32_t, const std::string&, int32_t,
            const std::string&, const std::string& error) {
            login_success = success;
            error_msg = error;
            done = true;
        });

    // Wait for response
    while (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    EXPECT_FALSE(login_success);
    EXPECT_EQ(error_msg, "Invalid password");
    EXPECT_FALSE(client->isAuthenticated());
}

TEST_F(E2EClientAuthTest, DISABLED_Login_UserNotFound) {
    std::atomic<bool> done{false};
    bool login_success = true;  // Will be set to false
    std::string error_msg;

    client->loginUser("nonexistent_user_12345", "anypassword",
        [&](bool success, uint32_t, const std::string&, int32_t,
            const std::string&, const std::string& error) {
            login_success = success;
            error_msg = error;
            done = true;
        });

    // Wait for response
    while (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    EXPECT_FALSE(login_success);
    EXPECT_EQ(error_msg, "User not found");
}

TEST_F(E2EClientAuthTest, DISABLED_Register_DuplicateUsername) {
    std::atomic<int> step{0};
    std::string username = "duplicate_test_" + std::to_string(time(nullptr));

    // First registration
    client->registerUser(username, "pass1", "User 1",
        [&](bool success, uint32_t, const std::string&) {
            EXPECT_TRUE(success);
            step = 1;
        });

    while (step < 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Second registration with same username
    bool reg2_success = true;
    std::string error_msg;
    client->registerUser(username, "pass2", "User 2",
        [&](bool success, uint32_t, const std::string& error) {
            reg2_success = success;
            error_msg = error;
            step = 2;
        });

    while (step < 2) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    EXPECT_FALSE(reg2_success);
    EXPECT_EQ(error_msg, "Username already exists");
}

TEST_F(E2EClientAuthTest, DISABLED_MultipleSequentialLogins) {
    // Register user
    std::string username = "multi_login_" + std::to_string(time(nullptr));
    std::atomic<int> step{0};
    uint32_t user_id = 0;

    client->registerUser(username, "password", "Multi Login Test",
        [&](bool success, uint32_t uid, const std::string&) {
            EXPECT_TRUE(success);
            user_id = uid;
            step = 1;
        });

    while (step < 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // First login
    client->loginUser(username, "password",
        [&](bool success, uint32_t uid, const std::string&, int32_t, const std::string&, const std::string&) {
            EXPECT_TRUE(success);
            EXPECT_EQ(uid, user_id);
            step = 2;
        });

    while (step < 2) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    EXPECT_TRUE(client->isAuthenticated());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Logout
    client->logoutUser([&](bool success) {
        EXPECT_TRUE(success);
        step = 3;
    });

    while (step < 3) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Second login
    client->loginUser(username, "password",
        [&](bool success, uint32_t uid, const std::string&, int32_t, const std::string&, const std::string&) {
            EXPECT_TRUE(success);
            EXPECT_EQ(uid, user_id);
            step = 4;
        });

    while (step < 4) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    EXPECT_TRUE(client->isAuthenticated());
}

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
