/**
 * Integration tests for authentication flow
 * Tests Register → Login → Logout with real server
 *
 * NOTE: These tests are DISABLED by default.
 * To run: Use run_integration_tests.sh script
 */

#include <gtest/gtest.h>
#include "protocol.h"
#include "messages/authentication_messages.h"
#include "message_serialization.h"
#include "config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>

using namespace MessageSerialization;

const char* TEST_SERVER_HOST = "127.0.0.1";
const int TIMEOUT_MS = 5000;

// Helper class for client connection
class TestClient {
public:
    TestClient() : socket_fd(-1) {}

    ~TestClient() {
        disconnect();
    }

    bool connect(const char* host, int port) {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            return false;
        }

        // Set timeout
        struct timeval tv;
        tv.tv_sec = TIMEOUT_MS / 1000;
        tv.tv_usec = (TIMEOUT_MS % 1000) * 1000;
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, host, &server_addr.sin_addr);

        if (::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(socket_fd);
            socket_fd = -1;
            return false;
        }

        return true;
    }

    void disconnect() {
        if (socket_fd >= 0) {
            close(socket_fd);
            socket_fd = -1;
        }
    }

    bool sendMessage(MessageType type, const std::string& payload) {
        MessageHeader header;
        header.type = static_cast<uint8_t>(type);
        header.length = payload.size();
        header.timestamp = time(nullptr);
        memset(header.session_token, 0, sizeof(header.session_token));

        // Send header
        if (send(socket_fd, &header, sizeof(header), 0) != sizeof(header)) {
            return false;
        }

        // Send payload if any
        if (payload.size() > 0) {
            if (send(socket_fd, payload.data(), payload.size(), 0) != (ssize_t)payload.size()) {
                return false;
            }
        }

        return true;
    }

    bool receiveMessage(MessageHeader& header, std::string& payload) {
        // Receive header
        ssize_t n = recv(socket_fd, &header, sizeof(header), MSG_WAITALL);
        if (n != sizeof(header)) {
            return false;
        }

        // Receive payload if any
        if (header.length > 0) {
            payload.resize(header.length);
            n = recv(socket_fd, &payload[0], header.length, MSG_WAITALL);
            if (n != (ssize_t)header.length) {
                return false;
            }
        } else {
            payload.clear();
        }

        return true;
    }

    // Receive message with expected type, skip other messages (like broadcasts)
    bool receiveExpectedMessage(MessageType expected_type, MessageHeader& header, std::string& payload) {
        int max_attempts = 10;  // Prevent infinite loop
        while (max_attempts-- > 0) {
            if (!receiveMessage(header, payload)) {
                return false;
            }
            if (header.type == static_cast<uint8_t>(expected_type)) {
                return true;
            }
            // Skip this message and try again
            std::cout << "[TEST] Skipping unexpected message type=" << (int)header.type 
                      << " (waiting for type=" << (int)expected_type << ")" << std::endl;
        }
        return false;
    }

private:
    int socket_fd;
};

// Base test fixture
class AuthIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        client = new TestClient();
    }

    void TearDown() override {
        delete client;
    }

    TestClient* client;
};

// ==================== Register Tests ====================

TEST_F(AuthIntegrationTest, Register_Success) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    // Generate unique username for this test
    std::string username = "testuser_" + std::to_string(time(nullptr));

    // Create register request
    RegisterRequest req;
    safeStrCopy(req.username, username.c_str(), sizeof(req.username));
    safeStrCopy(req.password, "hashed_password_123", sizeof(req.password));
    safeStrCopy(req.display_name, "Test User 1", sizeof(req.display_name));

    // Send register message
    ASSERT_TRUE(client->sendMessage(AUTH_REGISTER, serialize(req)));

    // Receive response
    MessageHeader resp_header;
    std::string resp_payload;
    ASSERT_TRUE(client->receiveMessage(resp_header, resp_payload));

    // Verify response type
    EXPECT_EQ(resp_header.type, AUTH_RESPONSE);

    // Deserialize response
    RegisterResponse resp;
    ASSERT_TRUE(deserialize(resp_payload, resp));

    // Verify success
    EXPECT_TRUE(resp.success);
    EXPECT_GT(resp.user_id, 0);
}

TEST_F(AuthIntegrationTest, Register_DuplicateUsername) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    // First registration
    RegisterRequest req1;
    safeStrCopy(req1.username, "duplicate_user", sizeof(req1.username));
    safeStrCopy(req1.password, "password1", sizeof(req1.password));
    safeStrCopy(req1.display_name, "User 1", sizeof(req1.display_name));

    ASSERT_TRUE(client->sendMessage(AUTH_REGISTER, serialize(req1)));

    MessageHeader resp1_header;
    std::string resp1_payload;
    ASSERT_TRUE(client->receiveMessage(resp1_header, resp1_payload));

    RegisterResponse resp1;
    ASSERT_TRUE(deserialize(resp1_payload, resp1));
    EXPECT_TRUE(resp1.success);

    // Second registration with same username
    client->disconnect();
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    RegisterRequest req2;
    safeStrCopy(req2.username, "duplicate_user", sizeof(req2.username));
    safeStrCopy(req2.password, "password2", sizeof(req2.password));
    safeStrCopy(req2.display_name, "User 2", sizeof(req2.display_name));

    ASSERT_TRUE(client->sendMessage(AUTH_REGISTER, serialize(req2)));

    MessageHeader resp2_header;
    std::string resp2_payload;
    ASSERT_TRUE(client->receiveMessage(resp2_header, resp2_payload));

    RegisterResponse resp2;
    ASSERT_TRUE(deserialize(resp2_payload, resp2));

    // Should fail
    EXPECT_FALSE(resp2.success);
    EXPECT_STREQ(resp2.error_message, "Username already exists");
}

// ==================== Login Tests ====================

TEST_F(AuthIntegrationTest, Login_Success) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    // Generate unique username for this test
    std::string username = "logintest_" + std::to_string(time(nullptr));

    // First register a user
    RegisterRequest reg_req;
    safeStrCopy(reg_req.username, username.c_str(), sizeof(reg_req.username));
    safeStrCopy(reg_req.password, "test_password_hash", sizeof(reg_req.password));
    safeStrCopy(reg_req.display_name, "Login Test User", sizeof(reg_req.display_name));

    ASSERT_TRUE(client->sendMessage(AUTH_REGISTER, serialize(reg_req)));

    MessageHeader reg_header;
    std::string reg_payload;
    ASSERT_TRUE(client->receiveMessage(reg_header, reg_payload));

    RegisterResponse reg_resp;
    ASSERT_TRUE(deserialize(reg_payload, reg_resp));
    ASSERT_TRUE(reg_resp.success);
    uint32_t user_id = reg_resp.user_id;

    // Now login
    client->disconnect();
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    LoginRequest login_req;
    safeStrCopy(login_req.username, username.c_str(), sizeof(login_req.username));
    safeStrCopy(login_req.password, "test_password_hash", sizeof(login_req.password));

    ASSERT_TRUE(client->sendMessage(AUTH_LOGIN, serialize(login_req)));

    MessageHeader login_header;
    std::string login_payload;
    ASSERT_TRUE(client->receiveMessage(login_header, login_payload));

    // Verify response
    EXPECT_EQ(login_header.type, AUTH_RESPONSE);

    LoginResponse login_resp;
    ASSERT_TRUE(deserialize(login_payload, login_resp));

    EXPECT_TRUE(login_resp.success);
    EXPECT_EQ(login_resp.user_id, user_id);
    EXPECT_STREQ(login_resp.display_name, "Login Test User");
    EXPECT_EQ(login_resp.elo_rating, 1000);  // Default starting ELO
    EXPECT_GT(strlen(login_resp.session_token), 0);  // Should have session token
}

TEST_F(AuthIntegrationTest, Login_WrongPassword) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    // Generate unique username for this test
    std::string username = "pwtest_" + std::to_string(time(nullptr));

    // Register user
    RegisterRequest reg_req;
    safeStrCopy(reg_req.username, username.c_str(), sizeof(reg_req.username));
    safeStrCopy(reg_req.password, "correct_password", sizeof(reg_req.password));
    safeStrCopy(reg_req.display_name, "PW Test", sizeof(reg_req.display_name));

    ASSERT_TRUE(client->sendMessage(AUTH_REGISTER, serialize(reg_req)));

    MessageHeader reg_header;
    std::string reg_payload;
    ASSERT_TRUE(client->receiveMessage(reg_header, reg_payload));

    // Login with wrong password
    client->disconnect();
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    LoginRequest login_req;
    safeStrCopy(login_req.username, username.c_str(), sizeof(login_req.username));
    safeStrCopy(login_req.password, "wrong_password", sizeof(login_req.password));

    ASSERT_TRUE(client->sendMessage(AUTH_LOGIN, serialize(login_req)));

    MessageHeader login_header;
    std::string login_payload;
    ASSERT_TRUE(client->receiveMessage(login_header, login_payload));

    LoginResponse login_resp;
    ASSERT_TRUE(deserialize(login_payload, login_resp));

    // Should fail
    EXPECT_FALSE(login_resp.success);
    EXPECT_STREQ(login_resp.error_message, "Invalid password");
}

TEST_F(AuthIntegrationTest, Login_UserNotFound) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    LoginRequest login_req;
    safeStrCopy(login_req.username, "nonexistent_user", sizeof(login_req.username));
    safeStrCopy(login_req.password, "any_password", sizeof(login_req.password));

    ASSERT_TRUE(client->sendMessage(AUTH_LOGIN, serialize(login_req)));

    MessageHeader login_header;
    std::string login_payload;
    ASSERT_TRUE(client->receiveMessage(login_header, login_payload));

    LoginResponse login_resp;
    ASSERT_TRUE(deserialize(login_payload, login_resp));

    // Should fail
    EXPECT_FALSE(login_resp.success);
    EXPECT_STREQ(login_resp.error_message, "User not found");
}

// ==================== Logout Tests ====================

TEST_F(AuthIntegrationTest, Logout_Success) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    // Generate unique username for this test
    std::string username = "logout_user_" + std::to_string(time(nullptr));
    
    // Register and login first
    RegisterRequest reg_req;
    safeStrCopy(reg_req.username, username.c_str(), sizeof(reg_req.username));
    safeStrCopy(reg_req.password, "password", sizeof(reg_req.password));
    safeStrCopy(reg_req.display_name, "Logout User", sizeof(reg_req.display_name));

    ASSERT_TRUE(client->sendMessage(AUTH_REGISTER, serialize(reg_req)));

    MessageHeader reg_header;
    std::string reg_payload;
    ASSERT_TRUE(client->receiveMessage(reg_header, reg_payload));

    // Login
    client->disconnect();
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    LoginRequest login_req;
    safeStrCopy(login_req.username, username.c_str(), sizeof(login_req.username));
    safeStrCopy(login_req.password, "password", sizeof(login_req.password));

    ASSERT_TRUE(client->sendMessage(AUTH_LOGIN, serialize(login_req)));

    MessageHeader login_header;
    std::string login_payload;
    ASSERT_TRUE(client->receiveExpectedMessage(AUTH_RESPONSE, login_header, login_payload));

    LoginResponse login_resp;
    ASSERT_TRUE(deserialize(login_payload, login_resp));
    ASSERT_TRUE(login_resp.success);

    // Now logout
    LogoutRequest logout_req;
    safeStrCopy(logout_req.session_token, login_resp.session_token, sizeof(logout_req.session_token));

    ASSERT_TRUE(client->sendMessage(AUTH_LOGOUT, serialize(logout_req)));

    MessageHeader logout_header;
    std::string logout_payload;
    ASSERT_TRUE(client->receiveExpectedMessage(AUTH_RESPONSE, logout_header, logout_payload));

    LogoutResponse logout_resp;
    ASSERT_TRUE(deserialize(logout_payload, logout_resp));

    EXPECT_TRUE(logout_resp.success);
}

// ==================== Complete Flow Test ====================

TEST_F(AuthIntegrationTest, CompleteAuthFlow) {
    // Test complete flow: Register → Login → Logout

    // Generate unique username for this test
    std::string username = "complete_flow_" + std::to_string(time(nullptr));

    // 1. Register
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    RegisterRequest reg_req;
    safeStrCopy(reg_req.username, username.c_str(), sizeof(reg_req.username));
    safeStrCopy(reg_req.password, "my_secure_password", sizeof(reg_req.password));
    safeStrCopy(reg_req.display_name, "Complete Flow User", sizeof(reg_req.display_name));

    ASSERT_TRUE(client->sendMessage(AUTH_REGISTER, serialize(reg_req)));

    MessageHeader reg_header;
    std::string reg_payload;
    ASSERT_TRUE(client->receiveMessage(reg_header, reg_payload));

    RegisterResponse reg_resp;
    ASSERT_TRUE(deserialize(reg_payload, reg_resp));
    ASSERT_TRUE(reg_resp.success);
    uint32_t user_id = reg_resp.user_id;

    // 2. Login
    client->disconnect();
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, SERVER_PORT));

    LoginRequest login_req;
    safeStrCopy(login_req.username, username.c_str(), sizeof(login_req.username));
    safeStrCopy(login_req.password, "my_secure_password", sizeof(login_req.password));

    ASSERT_TRUE(client->sendMessage(AUTH_LOGIN, serialize(login_req)));

    MessageHeader login_header;
    std::string login_payload;
    ASSERT_TRUE(client->receiveExpectedMessage(AUTH_RESPONSE, login_header, login_payload));

    LoginResponse login_resp;
    ASSERT_TRUE(deserialize(login_payload, login_resp));
    ASSERT_TRUE(login_resp.success);
    EXPECT_EQ(login_resp.user_id, user_id);

    // 3. Logout
    LogoutRequest logout_req;
    safeStrCopy(logout_req.session_token, login_resp.session_token, sizeof(logout_req.session_token));

    ASSERT_TRUE(client->sendMessage(AUTH_LOGOUT, serialize(logout_req)));

    MessageHeader logout_header;
    std::string logout_payload;
    ASSERT_TRUE(client->receiveExpectedMessage(AUTH_RESPONSE, logout_header, logout_payload));

    LogoutResponse logout_resp;
    ASSERT_TRUE(deserialize(logout_payload, logout_resp));
    EXPECT_TRUE(logout_resp.success);
}

// ==================== Main ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
