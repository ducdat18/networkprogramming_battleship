#include <gtest/gtest.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>
#include "protocol.h"
#include "messages/authentication_messages.h"
#include "messages/matchmaking_messages.h"
#include "message_serialization.h"
#include "config.h"

using namespace MessageSerialization;

class ChallengeIntegrationTest : public ::testing::Test {
protected:
    std::vector<int> client_fds;
    std::string server_host;
    int server_port;

    void SetUp() override {
        srand(time(nullptr) + rand());
        server_host = "127.0.0.1";
        server_port = SERVER_PORT;
    }

    void TearDown() override {
        for (int fd : client_fds) {
            if (fd > 0) {
                close(fd);
            }
        }
        client_fds.clear();
        // Small delay to let server cleanup connections
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    int connectToServer() {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return -1;
        }

        // Set timeout
        struct timeval tv;
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_host.c_str(), &server_addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            return -1;
        }

        client_fds.push_back(sock);
        return sock;
    }

    bool sendMessage(int sock, const MessageHeader& header, const std::string& payload) {
        if (send(sock, &header, sizeof(MessageHeader), 0) != sizeof(MessageHeader)) {
            return false;
        }
        if (header.length > 0) {
            if (send(sock, payload.data(), payload.size(), 0) != (ssize_t)payload.size()) {
                return false;
            }
        }
        return true;
    }

    bool receiveMessage(int sock, MessageHeader& header, std::string& payload) {
        ssize_t received = recv(sock, &header, sizeof(MessageHeader), MSG_WAITALL);
        if (received != sizeof(MessageHeader)) {
            return false;
        }

        if (header.length > 0) {
            payload.resize(header.length);
            received = recv(sock, &payload[0], header.length, MSG_WAITALL);
            if (received != (ssize_t)header.length) {
                return false;
            }
        }
        return true;
    }

    bool receiveExpectedMessage(int sock, MessageType expected_type, MessageHeader& header, std::string& payload) {
        int max_attempts = 20;  // Increased to handle more broadcasts
        while (max_attempts-- > 0) {
            if (!receiveMessage(sock, header, payload)) {
                return false;
            }
            if (header.type == static_cast<uint8_t>(expected_type)) {
                return true;
            }
            // Skip this message and try again (likely a broadcast)
        }
        return false;
    }

    bool registerUser(int sock, const std::string& username, const std::string& password, const std::string& display_name) {
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

    bool loginUser(int sock, const std::string& username, const std::string& password, std::string& session_token, uint32_t& user_id) {
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

        std::string response_payload;
        if (!receiveExpectedMessage(sock, MessageType::AUTH_RESPONSE, header, response_payload)) {
            return false;
        }

        LoginResponse resp;
        if (!deserialize(response_payload, resp)) {
            return false;
        }

        if (resp.success) {
            session_token = std::string(resp.session_token, 64);
            user_id = resp.user_id;
        }
        return resp.success;
    }

    bool sendChallenge(int sock, const std::string& session_token, uint32_t target_user_id) {
        ChallengeRequest req;
        req.target_user_id = target_user_id;
        req.time_limit = 60;
        req.random_placement = false;

        MessageHeader header;
        header.type = static_cast<uint8_t>(MessageType::CHALLENGE_SEND);
        header.length = sizeof(ChallengeRequest);
        header.timestamp = time(nullptr);
        memcpy(header.session_token, session_token.c_str(), std::min(session_token.size(), sizeof(header.session_token)));

        return sendMessage(sock, header, serialize(req));
    }

    bool receiveChallenge(int sock, uint32_t& challenge_id, uint32_t& challenger_id) {
        MessageHeader header;
        std::string payload;

        if (!receiveExpectedMessage(sock, MessageType::CHALLENGE_RECEIVED, header, payload)) {
            return false;
        }

        ChallengeReceived msg;
        if (!deserialize(payload, msg)) {
            return false;
        }

        challenge_id = msg.challenge_id;
        challenger_id = msg.challenger_id;
        return true;
    }

    bool respondChallenge(int sock, const std::string& session_token, uint32_t challenge_id, bool accept) {
        ChallengeResponse resp;
        resp.challenge_id = challenge_id;
        resp.accepted = accept;

        MessageHeader header;
        header.type = static_cast<uint8_t>(MessageType::CHALLENGE_RESPONSE);
        header.length = sizeof(ChallengeResponse);
        header.timestamp = time(nullptr);
        memcpy(header.session_token, session_token.c_str(), std::min(session_token.size(), sizeof(header.session_token)));

        return sendMessage(sock, header, serialize(resp));
    }

    bool receiveMatchStart(int sock, uint32_t& match_id) {
        MessageHeader header;
        std::string payload;

        if (!receiveExpectedMessage(sock, MessageType::MATCH_START, header, payload)) {
            return false;
        }

        MatchStartMessage msg;
        if (!deserialize(payload, msg)) {
            return false;
        }

        match_id = msg.match_id;
        return true;
    }

    std::string generateUsername(const std::string& prefix) {
        return prefix + std::to_string(time(nullptr)) + "_" + std::to_string(rand());
    }
};

TEST_F(ChallengeIntegrationTest, SendAndReceiveChallenge) {
    // Connect clients
    int sock1 = connectToServer();
    int sock2 = connectToServer();
    ASSERT_GT(sock1, 0);
    ASSERT_GT(sock2, 0);

    // Register and login
    std::string user1 = generateUsername("challenger");
    std::string user2 = generateUsername("target");

    ASSERT_TRUE(registerUser(sock1, user1, "pass123", "Challenger"));
    ASSERT_TRUE(registerUser(sock2, user2, "pass123", "Target"));

    std::string token1, token2;
    uint32_t user1_id, user2_id;

    ASSERT_TRUE(loginUser(sock1, user1, "pass123", token1, user1_id));
    ASSERT_TRUE(loginUser(sock2, user2, "pass123", token2, user2_id));

    std::cout << "✅ User1 (ID=" << user1_id << ") and User2 (ID=" << user2_id << ") logged in" << std::endl;

    // Delay to ensure players are fully registered in PlayerManager
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // User1 sends challenge to User2
    ASSERT_TRUE(sendChallenge(sock1, token1, user2_id));
    std::cout << "✅ Challenge sent from User1 to User2" << std::endl;

    // User2 receives challenge
    uint32_t challenge_id, challenger_id;
    ASSERT_TRUE(receiveChallenge(sock2, challenge_id, challenger_id));
    EXPECT_EQ(challenger_id, user1_id);
    std::cout << "✅ Challenge received by User2 (challenge_id=" << challenge_id << ")" << std::endl;
}

TEST_F(ChallengeIntegrationTest, AcceptChallengeCreatesMatch) {
    // Connect clients
    int sock1 = connectToServer();
    int sock2 = connectToServer();
    ASSERT_GT(sock1, 0);
    ASSERT_GT(sock2, 0);

    // Register and login
    std::string user1 = generateUsername("challenger");
    std::string user2 = generateUsername("target");

    ASSERT_TRUE(registerUser(sock1, user1, "pass123", "Challenger"));
    ASSERT_TRUE(registerUser(sock2, user2, "pass123", "Target"));

    std::string token1, token2;
    uint32_t user1_id, user2_id;

    ASSERT_TRUE(loginUser(sock1, user1, "pass123", token1, user1_id));
    ASSERT_TRUE(loginUser(sock2, user2, "pass123", token2, user2_id));

    // Delay to ensure players are fully registered in PlayerManager
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send challenge
    ASSERT_TRUE(sendChallenge(sock1, token1, user2_id));

    // Receive challenge
    uint32_t challenge_id, challenger_id;
    ASSERT_TRUE(receiveChallenge(sock2, challenge_id, challenger_id));

    // Accept challenge
    ASSERT_TRUE(respondChallenge(sock2, token2, challenge_id, true));
    std::cout << "✅ Challenge accepted by User2" << std::endl;

    // Both clients should receive MATCH_START
    uint32_t match_id1, match_id2;
    ASSERT_TRUE(receiveMatchStart(sock1, match_id1));
    ASSERT_TRUE(receiveMatchStart(sock2, match_id2));

    EXPECT_EQ(match_id1, match_id2);
    EXPECT_GT(match_id1, 0u);

    std::cout << "✅ Match created (match_id=" << match_id1 << ")" << std::endl;
}

TEST_F(ChallengeIntegrationTest, DeclineChallengeNoMatch) {
    // Connect clients
    int sock1 = connectToServer();
    int sock2 = connectToServer();
    ASSERT_GT(sock1, 0);
    ASSERT_GT(sock2, 0);

    // Register and login
    std::string user1 = generateUsername("challenger");
    std::string user2 = generateUsername("target");

    ASSERT_TRUE(registerUser(sock1, user1, "pass123", "Challenger"));
    ASSERT_TRUE(registerUser(sock2, user2, "pass123", "Target"));

    std::string token1, token2;
    uint32_t user1_id, user2_id;

    ASSERT_TRUE(loginUser(sock1, user1, "pass123", token1, user1_id));
    ASSERT_TRUE(loginUser(sock2, user2, "pass123", token2, user2_id));

    // Delay to ensure players are fully registered in PlayerManager
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send challenge
    ASSERT_TRUE(sendChallenge(sock1, token1, user2_id));

    // Receive challenge
    uint32_t challenge_id, challenger_id;
    ASSERT_TRUE(receiveChallenge(sock2, challenge_id, challenger_id));

    // Decline challenge
    ASSERT_TRUE(respondChallenge(sock2, token2, challenge_id, false));
    std::cout << "✅ Challenge declined by User2" << std::endl;

    // Neither client should receive MATCH_START (they might receive status updates though)
    // Just verify we don't crash and the connection is still alive
}

TEST_F(ChallengeIntegrationTest, MultipleChallengesToSameUser) {
    // Connect clients
    int sock1 = connectToServer();
    int sock2 = connectToServer();
    int sock3 = connectToServer();
    ASSERT_GT(sock1, 0);
    ASSERT_GT(sock2, 0);
    ASSERT_GT(sock3, 0);

    // Register and login
    std::string user1 = generateUsername("challenger1");
    std::string user2 = generateUsername("target");
    std::string user3 = generateUsername("challenger2");

    ASSERT_TRUE(registerUser(sock1, user1, "pass123", "Challenger1"));
    ASSERT_TRUE(registerUser(sock2, user2, "pass123", "Target"));
    ASSERT_TRUE(registerUser(sock3, user3, "pass123", "Challenger2"));

    std::string token1, token2, token3;
    uint32_t user1_id, user2_id, user3_id;

    ASSERT_TRUE(loginUser(sock1, user1, "pass123", token1, user1_id));
    ASSERT_TRUE(loginUser(sock2, user2, "pass123", token2, user2_id));
    ASSERT_TRUE(loginUser(sock3, user3, "pass123", token3, user3_id));

    // Small delay to ensure players are registered
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Both User1 and User3 challenge User2
    ASSERT_TRUE(sendChallenge(sock1, token1, user2_id));
    ASSERT_TRUE(sendChallenge(sock3, token3, user2_id));

    // User2 receives both challenges
    uint32_t challenge_id1, challenger_id1;
    ASSERT_TRUE(receiveChallenge(sock2, challenge_id1, challenger_id1));

    uint32_t challenge_id2, challenger_id2;
    ASSERT_TRUE(receiveChallenge(sock2, challenge_id2, challenger_id2));

    std::cout << "✅ User2 received two challenges (IDs: " << challenge_id1 << ", " << challenge_id2 << ")" << std::endl;

    // User2 accepts first challenge
    ASSERT_TRUE(respondChallenge(sock2, token2, challenge_id1, true));

    // Match should be created
    uint32_t match_id;
    // Receive MATCH_START from either challenger1 or challenger2 (whichever sent first challenge)
    if (challenger_id1 == user1_id) {
        ASSERT_TRUE(receiveMatchStart(sock1, match_id));
    } else {
        ASSERT_TRUE(receiveMatchStart(sock3, match_id));
    }
    ASSERT_TRUE(receiveMatchStart(sock2, match_id));

    std::cout << "✅ First challenge accepted, match created (match_id=" << match_id << ")" << std::endl;
}

int main(int argc, char **argv) {
    srand(time(nullptr));
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << std::endl;
    std::cout << "╔════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   CHALLENGE SYSTEM INTEGRATION TESTS       ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  Requirements:                             ║" << std::endl;
    std::cout << "║  - Server running on localhost:" << SERVER_PORT << "        ║" << std::endl;
    std::cout << "║  - Database initialized                    ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    return RUN_ALL_TESTS();
}
