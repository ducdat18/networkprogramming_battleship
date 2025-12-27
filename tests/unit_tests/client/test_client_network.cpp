/**
 * Unit tests for ClientNetwork class
 * Tests connection, authentication methods, and callbacks
 */

#include <gtest/gtest.h>
#include "client_network.h"
#include "message_serialization.h"
#include <thread>
#include <chrono>

using namespace MessageSerialization;

class ClientNetworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        client = new ClientNetwork();
    }

    void TearDown() override {
        delete client;
    }

    ClientNetwork* client;
};

// ==================== Basic Tests ====================

TEST_F(ClientNetworkTest, InitialState) {
    EXPECT_FALSE(client->isConnected());
    EXPECT_FALSE(client->isAuthenticated());
    EXPECT_EQ(client->getStatus(), ClientNetwork::DISCONNECTED);
    EXPECT_EQ(client->getUserId(), 0);
}

TEST_F(ClientNetworkTest, ConnectionFailure_InvalidHost) {
    bool callback_called = false;
    bool connection_success = false;

    client->connect("invalid.host.local", 9999, [&](bool success, const std::string& error) {
        callback_called = true;
        connection_success = success;
    });

    // Give it a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(connection_success);
    EXPECT_FALSE(client->isConnected());
}

TEST_F(ClientNetworkTest, ConnectionFailure_InvalidPort) {
    bool callback_called = false;
    bool connection_success = false;

    // Port 1 is usually restricted/not listening
    client->connect("127.0.0.1", 1, [&](bool success, const std::string& error) {
        callback_called = true;
        connection_success = success;
    });

    // Give it a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(connection_success);
    EXPECT_FALSE(client->isConnected());
}

TEST_F(ClientNetworkTest, Disconnect_WhenNotConnected) {
    // Should not crash
    client->disconnect();
    EXPECT_FALSE(client->isConnected());
}

TEST_F(ClientNetworkTest, SessionInfo_BeforeAuth) {
    EXPECT_EQ(client->getUserId(), 0);
    EXPECT_EQ(client->getSessionToken(), "");
    EXPECT_EQ(client->getDisplayName(), "");
    EXPECT_EQ(client->getEloRating(), 0);
}

// ==================== Authentication Method Tests ====================

TEST_F(ClientNetworkTest, RegisterUser_NotConnected) {
    bool callback_called = false;
    bool reg_success = false;
    std::string error_msg;

    client->registerUser("testuser", "password", "Test User",
        [&](bool success, uint32_t user_id, const std::string& error) {
            callback_called = true;
            reg_success = success;
            error_msg = error;
        });

    // Give callback time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(reg_success);
    EXPECT_EQ(error_msg, "Not connected to server");
}

TEST_F(ClientNetworkTest, LoginUser_NotConnected) {
    bool callback_called = false;
    bool login_success = false;
    std::string error_msg;

    client->loginUser("testuser", "password",
        [&](bool success, uint32_t user_id, const std::string& display_name,
            int32_t elo_rating, const std::string& session_token, const std::string& error) {
            callback_called = true;
            login_success = success;
            error_msg = error;
        });

    // Give callback time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(login_success);
    EXPECT_EQ(error_msg, "Not connected to server");
}

TEST_F(ClientNetworkTest, LogoutUser_NotAuthenticated) {
    bool callback_called = false;
    bool logout_success = true;  // Will be set to false

    client->logoutUser([&](bool success) {
        callback_called = true;
        logout_success = success;
    });

    // Give callback time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(logout_success);
}

// ==================== Message Serialization Tests ====================

TEST_F(ClientNetworkTest, MessageSerialization_RegisterRequest) {
    RegisterRequest req;
    safeStrCopy(req.username, "testuser", sizeof(req.username));
    safeStrCopy(req.password, "password123", sizeof(req.password));
    safeStrCopy(req.display_name, "Test User", sizeof(req.display_name));

    std::string serialized = serialize(req);
    EXPECT_EQ(serialized.size(), sizeof(RegisterRequest));

    RegisterRequest deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));
    EXPECT_STREQ(deserialized.username, "testuser");
    EXPECT_STREQ(deserialized.password, "password123");
    EXPECT_STREQ(deserialized.display_name, "Test User");
}

TEST_F(ClientNetworkTest, MessageSerialization_LoginRequest) {
    LoginRequest req;
    safeStrCopy(req.username, "user123", sizeof(req.username));
    safeStrCopy(req.password, "pass456", sizeof(req.password));

    std::string serialized = serialize(req);
    EXPECT_EQ(serialized.size(), sizeof(LoginRequest));

    LoginRequest deserialized;
    ASSERT_TRUE(deserialize(serialized, deserialized));
    EXPECT_STREQ(deserialized.username, "user123");
    EXPECT_STREQ(deserialized.password, "pass456");
}

// ==================== State Tests ====================

TEST_F(ClientNetworkTest, ConnectionStatus_Transitions) {
    EXPECT_EQ(client->getStatus(), ClientNetwork::DISCONNECTED);

    // After disconnect (already disconnected)
    client->disconnect();
    EXPECT_EQ(client->getStatus(), ClientNetwork::DISCONNECTED);
}

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
