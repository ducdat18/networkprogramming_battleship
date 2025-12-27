#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#include "protocol.h"
#include "messages/authentication_messages.h"
#include "messages/matchmaking_messages.h"
#include "message_serialization.h"
#include "password_hash.h"
#include "config.h"

using namespace MessageSerialization;

/**
 * Integration Tests for Player List Feature
 *
 * Tests:
 * - Login registers player with PlayerManager
 * - PLAYER_LIST_REQUEST returns online players
 * - Multiple clients can see each other
 * - Logout removes player from list
 * - Player status updates are broadcasted
 */

class PlayerListIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        srand(time(nullptr) + rand());  // Seed random for unique usernames
        server_host = "127.0.0.1";
        server_port = SERVER_PORT;
    }

    void TearDown() override {
        // Close any open sockets
        for (int fd : client_fds) {
            if (fd > 0) {
                close(fd);
            }
        }
        client_fds.clear();
    }

    int connectToServer() {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return -1;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_host.c_str(), &server_addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to connect to server" << std::endl;
            close(sock);
            return -1;
        }

        client_fds.push_back(sock);
        return sock;
    }

    bool sendMessage(int sock, const MessageHeader& header, const std::string& payload) {
        // Send header
        if (send(sock, &header, sizeof(MessageHeader), 0) != sizeof(MessageHeader)) {
            return false;
        }

        // Send payload
        if (header.length > 0) {
            if (send(sock, payload.data(), payload.size(), 0) != (ssize_t)payload.size()) {
                return false;
            }
        }

        return true;
    }

    bool receiveMessage(int sock, MessageHeader& header, std::string& payload) {
        // Receive header
        ssize_t received = recv(sock, &header, sizeof(MessageHeader), MSG_WAITALL);
        if (received != sizeof(MessageHeader)) {
            return false;
        }

        // Receive payload
        if (header.length > 0) {
            payload.resize(header.length);
            received = recv(sock, &payload[0], header.length, MSG_WAITALL);
            if (received != (ssize_t)header.length) {
                return false;
            }
        }

        return true;
    }

    // Receive message with expected type, skip other messages (like broadcasts)
    bool receiveExpectedMessage(int sock, MessageType expected_type, MessageHeader& header, std::string& payload) {
        int max_attempts = 10;  // Prevent infinite loop
        while (max_attempts-- > 0) {
            if (!receiveMessage(sock, header, payload)) {
                return false;
            }
            if (header.type == static_cast<uint8_t>(expected_type)) {
                return true;
            }
            // Skip this message and try again
        }
        return false;
    }

    bool registerUser(int sock, const std::string& username, const std::string& password,
                     const std::string& display_name) {
        RegisterRequest req;
        safeStrCopy(req.username, username, sizeof(req.username));
        safeStrCopy(req.password, password, sizeof(req.password));
        safeStrCopy(req.display_name, display_name, sizeof(req.display_name));

        MessageHeader header;
        header.type = static_cast<uint8_t>(MessageType::AUTH_REGISTER);
        header.length = sizeof(RegisterRequest);
        header.timestamp = time(nullptr);
        memset(header.session_token, 0, sizeof(header.session_token));

        if (!sendMessage(sock, header, serialize(req))) {
            return false;
        }

        // Receive response (skip broadcast messages)
        std::string response_payload;
        if (!receiveExpectedMessage(sock, MessageType::AUTH_RESPONSE, header, response_payload)) {
            return false;
        }

        RegisterResponse resp;
        if (!deserialize(response_payload, resp)) {
            return false;
        }

        return resp.success;
    }

    bool loginUser(int sock, const std::string& username, const std::string& password,
                  std::string& session_token) {
        LoginRequest req;
        safeStrCopy(req.username, username, sizeof(req.username));
        safeStrCopy(req.password, password, sizeof(req.password));

        MessageHeader header;
        header.type = static_cast<uint8_t>(MessageType::AUTH_LOGIN);
        header.length = sizeof(LoginRequest);
        header.timestamp = time(nullptr);
        memset(header.session_token, 0, sizeof(header.session_token));

        if (!sendMessage(sock, header, serialize(req))) {
            return false;
        }

        // Receive response (skip broadcast messages)
        std::string response_payload;
        if (!receiveExpectedMessage(sock, MessageType::AUTH_RESPONSE, header, response_payload)) {
            return false;
        }

        LoginResponse resp;
        if (!deserialize(response_payload, resp)) {
            return false;
        }

        if (resp.success) {
            session_token = resp.session_token;
        }

        return resp.success;
    }

    bool requestPlayerList(int sock, PlayerListResponse& response) {
        PlayerListRequest req;

        MessageHeader header;
        header.type = static_cast<uint8_t>(MessageType::PLAYER_LIST_REQUEST);
        header.length = sizeof(PlayerListRequest);
        header.timestamp = time(nullptr);
        memset(header.session_token, 0, sizeof(header.session_token));

        if (!sendMessage(sock, header, serialize(req))) {
            return false;
        }

        // Receive response (skip broadcast messages)
        std::string response_payload;
        if (!receiveExpectedMessage(sock, MessageType::PLAYER_LIST, header, response_payload)) {
            return false;
        }

        if (!deserialize(response_payload, response)) {
            return false;
        }

        return true;
    }

    std::string server_host;
    int server_port;
    std::vector<int> client_fds;
};

// Test: Login registers player
TEST_F(PlayerListIntegrationTest, LoginRegistersPlayer) {
    int sock = connectToServer();
    ASSERT_GT(sock, 0);

    // Generate unique username with both timestamp and random component
    std::string username = "test_login_" + std::to_string(time(nullptr)) + "_" + std::to_string(rand() % 10000);
    std::string password = "password123";
    std::string display_name = "Test Player 1";

    ASSERT_TRUE(registerUser(sock, username, password, display_name));

    std::string session_token;
    ASSERT_TRUE(loginUser(sock, username, password, session_token));
    EXPECT_FALSE(session_token.empty());

    std::cout << "✅ Login registered player successfully" << std::endl;
}

// Test: Request player list returns online players
TEST_F(PlayerListIntegrationTest, RequestPlayerListReturnsPlayers) {
    int sock = connectToServer();
    ASSERT_GT(sock, 0);

    // Generate unique username with both timestamp and random component
    std::string username = "test_plist_" + std::to_string(time(nullptr)) + "_" + std::to_string(rand() % 10000);
    std::string password = "password123";
    std::string display_name = "Test Player 2";

    ASSERT_TRUE(registerUser(sock, username, password, display_name));

    std::string session_token;
    ASSERT_TRUE(loginUser(sock, username, password, session_token));

    // Request player list
    PlayerListResponse response;
    ASSERT_TRUE(requestPlayerList(sock, response));

    // Should have at least 1 player (ourselves)
    EXPECT_GT(response.count, 0);

    std::cout << "✅ Player list returned " << response.count << " players" << std::endl;
}

// Test: Multiple clients see each other
TEST_F(PlayerListIntegrationTest, MultipleClientsSeeEachOther) {
    const int num_clients = 3;
    std::vector<int> socks;
    std::vector<std::string> usernames;
    std::string ts = std::to_string(time(nullptr));

    // Connect and login multiple clients
    for (int i = 0; i < num_clients; i++) {
        int sock = connectToServer();
        ASSERT_GT(sock, 0);
        socks.push_back(sock);

        std::string username = "test_multi_" + ts + "_" + std::to_string(i);
        std::string password = "password123";
        std::string display_name = "Multi Player " + std::to_string(i);

        ASSERT_TRUE(registerUser(sock, username, password, display_name));

        std::string session_token;
        ASSERT_TRUE(loginUser(sock, username, password, session_token));

        usernames.push_back(username);

        std::cout << "Client " << i << " logged in: " << username << std::endl;
    }

    // Wait a bit for all players to be registered
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Each client requests player list
    for (int i = 0; i < num_clients; i++) {
        PlayerListResponse response;
        ASSERT_TRUE(requestPlayerList(socks[i], response));

        // Should see all players (including themselves)
        EXPECT_GE(response.count, num_clients);

        std::cout << "Client " << i << " sees " << response.count << " players" << std::endl;

        // Verify all our test players are in the list
        for (const std::string& expected_user : usernames) {
            bool found = false;
            for (uint32_t j = 0; j < response.count; j++) {
                if (std::string(response.players[j].username) == expected_user) {
                    found = true;
                    EXPECT_EQ(response.players[j].status, STATUS_AVAILABLE);
                    break;
                }
            }
            EXPECT_TRUE(found) << "Player " << expected_user << " not found in list";
        }
    }

    std::cout << "✅ All clients can see each other" << std::endl;
}

// Test: Logout removes player from list
TEST_F(PlayerListIntegrationTest, LogoutRemovesPlayer) {
    // Generate unique usernames
    std::string ts = std::to_string(time(nullptr));
    std::string username1 = "test_logout_1_" + ts;
    std::string username2 = "test_logout_2_" + ts;

    // Client 1 - will logout
    int sock1 = connectToServer();
    ASSERT_GT(sock1, 0);

    ASSERT_TRUE(registerUser(sock1, username1, "password123", "Logout Test 1"));
    std::string token1;
    ASSERT_TRUE(loginUser(sock1, username1, "password123", token1));

    // Client 2 - will check player list
    int sock2 = connectToServer();
    ASSERT_GT(sock2, 0);

    ASSERT_TRUE(registerUser(sock2, username2, "password123", "Logout Test 2"));
    std::string token2;
    ASSERT_TRUE(loginUser(sock2, username2, "password123", token2));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Client 2 checks player list (should see both)
    PlayerListResponse response1;
    ASSERT_TRUE(requestPlayerList(sock2, response1));
    uint32_t count_before = response1.count;

    std::cout << "Before logout: " << count_before << " players" << std::endl;

    // Client 1 logs out
    LogoutRequest logout_req;
    safeStrCopy(logout_req.session_token, token1, sizeof(logout_req.session_token));

    MessageHeader logout_header;
    logout_header.type = static_cast<uint8_t>(MessageType::AUTH_LOGOUT);
    logout_header.length = sizeof(LogoutRequest);
    logout_header.timestamp = time(nullptr);
    memset(logout_header.session_token, 0, sizeof(logout_header.session_token));

    ASSERT_TRUE(sendMessage(sock1, logout_header, serialize(logout_req)));

    // Receive logout response (skip broadcast messages)
    std::string logout_response;
    ASSERT_TRUE(receiveExpectedMessage(sock1, MessageType::AUTH_RESPONSE, logout_header, logout_response));

    // Wait for server to process logout and update player list
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    close(sock1);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Client 2 checks player list again (should not see client 1)
    PlayerListResponse response2;
    ASSERT_TRUE(requestPlayerList(sock2, response2));

    std::cout << "After logout: " << response2.count << " players" << std::endl;

    // Verify client 1 is not in the list
    bool found_client1 = false;
    for (uint32_t i = 0; i < response2.count; i++) {
        if (std::string(response2.players[i].username) == username1) {
            found_client1 = true;
            break;
        }
    }
    EXPECT_FALSE(found_client1) << "Client 1 should not be in player list after logout";

    std::cout << "✅ Logout removes player from list" << std::endl;
}

// Test: Player list contains correct information
TEST_F(PlayerListIntegrationTest, PlayerListContainsCorrectInfo) {
    int sock = connectToServer();
    ASSERT_GT(sock, 0);

    std::string username = "test_info_" + std::to_string(time(nullptr));
    std::string display_name = "Info Check Player";
    ASSERT_TRUE(registerUser(sock, username, "password123", display_name));

    std::string token;
    ASSERT_TRUE(loginUser(sock, username, "password123", token));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Request player list
    PlayerListResponse response;
    ASSERT_TRUE(requestPlayerList(sock, response));

    // Find our player in the list
    bool found = false;
    for (uint32_t i = 0; i < response.count; i++) {
        if (std::string(response.players[i].username) == username) {
            found = true;
            EXPECT_STREQ(response.players[i].display_name, display_name.c_str());
            EXPECT_EQ(response.players[i].status, STATUS_AVAILABLE);
            EXPECT_EQ(response.players[i].elo_rating, 1000); // Default ELO
            break;
        }
    }

    EXPECT_TRUE(found) << "Our player should be in the list";
    std::cout << "✅ Player list contains correct info" << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
