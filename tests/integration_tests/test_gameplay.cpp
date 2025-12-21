#include <gtest/gtest.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include "protocol.h"
#include "messages/authentication_messages.h"
#include "messages/matchmaking_messages.h"
#include "messages/gameplay_messages.h"
#include "config.h"

class GameplayTest : public ::testing::Test {
protected:
    int client1_fd = -1;
    int client2_fd = -1;
    uint32_t user1_id = 0;
    uint32_t user2_id = 0;
    uint32_t match_id = 0;
    std::string session1_token;
    std::string session2_token;

    void SetUp() override {
        // Connect both clients
        client1_fd = connectToServer();
        ASSERT_GT(client1_fd, 0) << "Client 1 failed to connect";

        client2_fd = connectToServer();
        ASSERT_GT(client2_fd, 0) << "Client 2 failed to connect";

        // Small delay for server to process connections
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (client1_fd > 0) close(client1_fd);
        if (client2_fd > 0) close(client2_fd);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    int connectToServer() {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) return -1;

        struct timeval tv;
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_HOST, &server_addr.sin_addr);

        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sockfd);
            return -1;
        }

        return sockfd;
    }

    bool sendMessage(int sockfd, MessageType type, const void* payload, size_t payload_size) {
        MessageHeader header;
        memset(&header, 0, sizeof(header));
        header.type = type;
        header.length = payload_size;
        header.timestamp = time(nullptr);

        if (send(sockfd, &header, sizeof(header), 0) != sizeof(header)) {
            return false;
        }

        if (payload_size > 0 && send(sockfd, payload, payload_size, 0) != (ssize_t)payload_size) {
            return false;
        }

        return true;
    }

    bool receiveMessage(int sockfd, MessageHeader& header, void* payload, size_t max_payload_size) {
        ssize_t received = recv(sockfd, &header, sizeof(header), 0);
        if (received != sizeof(header)) {
            return false;
        }

        if (header.length > 0 && header.length <= max_payload_size) {
            received = recv(sockfd, payload, header.length, 0);
            if (received != (ssize_t)header.length) {
                return false;
            }
        }

        return true;
    }

    bool skipBroadcastMessages(int sockfd, MessageType expected_type, void* payload, size_t max_payload_size, int max_attempts = 20) {
        MessageHeader header;
        for (int i = 0; i < max_attempts; i++) {
            if (!receiveMessage(sockfd, header, payload, max_payload_size)) {
                return false;
            }

            if (header.type == expected_type) {
                return true;
            }

            // Skip PLAYER_STATUS_UPDATE broadcasts
            if (header.type == MessageType::PLAYER_STATUS_UPDATE) {
                continue;
            }

            // Unexpected message
            std::cerr << "Unexpected message type: " << header.type << " (expected: " << expected_type << ")" << std::endl;
            return false;
        }
        return false;
    }
};

TEST_F(GameplayTest, CompleteGameplayFlow) {
    // Step 1: Register and login both players
    std::cout << "\n=== Step 1: Register and login players ===" << std::endl;

    // Generate unique usernames
    long timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    std::string username1 = "player1_" + std::to_string(timestamp) + "_" + std::to_string(rand());
    std::string username2 = "player2_" + std::to_string(timestamp) + "_" + std::to_string(rand());

    // Register player 1
    RegisterRequest reg1;
    memset(&reg1, 0, sizeof(reg1));
    strncpy(reg1.username, username1.c_str(), sizeof(reg1.username) - 1);
    strncpy(reg1.password, "pass123", sizeof(reg1.password) - 1);
    strncpy(reg1.display_name, "Player One", sizeof(reg1.display_name) - 1);

    ASSERT_TRUE(sendMessage(client1_fd, MessageType::REGISTER, &reg1, sizeof(reg1)));

    RegisterResponse regResp1;
    ASSERT_TRUE(skipBroadcastMessages(client1_fd, MessageType::REGISTER_RESPONSE, &regResp1, sizeof(regResp1)));
    ASSERT_TRUE(regResp1.success);
    user1_id = regResp1.user_id;
    std::cout << "Player 1 registered: user_id=" << user1_id << std::endl;

    // Login player 1
    LoginRequest login1;
    memset(&login1, 0, sizeof(login1));
    strncpy(login1.username, username1.c_str(), sizeof(login1.username) - 1);
    strncpy(login1.password, "pass123", sizeof(login1.password) - 1);

    ASSERT_TRUE(sendMessage(client1_fd, MessageType::LOGIN, &login1, sizeof(login1)));

    LoginResponse loginResp1;
    ASSERT_TRUE(skipBroadcastMessages(client1_fd, MessageType::LOGIN_RESPONSE, &loginResp1, sizeof(loginResp1)));
    ASSERT_TRUE(loginResp1.success);
    session1_token = loginResp1.session_token;
    std::cout << "Player 1 logged in" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Register player 2
    RegisterRequest reg2;
    memset(&reg2, 0, sizeof(reg2));
    strncpy(reg2.username, username2.c_str(), sizeof(reg2.username) - 1);
    strncpy(reg2.password, "pass456", sizeof(reg2.password) - 1);
    strncpy(reg2.display_name, "Player Two", sizeof(reg2.display_name) - 1);

    ASSERT_TRUE(sendMessage(client2_fd, MessageType::REGISTER, &reg2, sizeof(reg2)));

    RegisterResponse regResp2;
    ASSERT_TRUE(skipBroadcastMessages(client2_fd, MessageType::REGISTER_RESPONSE, &regResp2, sizeof(regResp2)));
    ASSERT_TRUE(regResp2.success);
    user2_id = regResp2.user_id;
    std::cout << "Player 2 registered: user_id=" << user2_id << std::endl;

    // Login player 2
    LoginRequest login2;
    memset(&login2, 0, sizeof(login2));
    strncpy(login2.username, username2.c_str(), sizeof(login2.username) - 1);
    strncpy(login2.password, "pass456", sizeof(login2.password) - 1);

    ASSERT_TRUE(sendMessage(client2_fd, MessageType::LOGIN, &login2, sizeof(login2)));

    LoginResponse loginResp2;
    ASSERT_TRUE(skipBroadcastMessages(client2_fd, MessageType::LOGIN_RESPONSE, &loginResp2, sizeof(loginResp2)));
    ASSERT_TRUE(loginResp2.success);
    session2_token = loginResp2.session_token;
    std::cout << "Player 2 logged in" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Step 2: Player 1 challenges Player 2
    std::cout << "\n=== Step 2: Send challenge ===" << std::endl;

    ChallengeSend challenge;
    memset(&challenge, 0, sizeof(challenge));
    challenge.target_user_id = user2_id;
    challenge.time_limit = 60;
    challenge.random_placement = false;

    ASSERT_TRUE(sendMessage(client1_fd, MessageType::CHALLENGE_SEND, &challenge, sizeof(challenge)));
    std::cout << "Player 1 sent challenge to Player 2" << std::endl;

    // Player 2 receives challenge
    ChallengeReceived challengeRecv;
    ASSERT_TRUE(skipBroadcastMessages(client2_fd, MessageType::CHALLENGE_RECEIVED, &challengeRecv, sizeof(challengeRecv)));
    uint32_t challenge_id = challengeRecv.challenge_id;
    std::cout << "Player 2 received challenge: challenge_id=" << challenge_id << std::endl;

    // Step 3: Player 2 accepts challenge
    std::cout << "\n=== Step 3: Accept challenge ===" << std::endl;

    ChallengeResponse response;
    memset(&response, 0, sizeof(response));
    response.challenge_id = challenge_id;
    response.accept = true;

    ASSERT_TRUE(sendMessage(client2_fd, MessageType::CHALLENGE_RESPONSE, &response, sizeof(response)));
    std::cout << "Player 2 accepted challenge" << std::endl;

    // Both players receive MATCH_START
    MatchStartMessage matchStart1, matchStart2;
    ASSERT_TRUE(skipBroadcastMessages(client1_fd, MessageType::MATCH_START, &matchStart1, sizeof(matchStart1)));
    ASSERT_TRUE(skipBroadcastMessages(client2_fd, MessageType::MATCH_START, &matchStart2, sizeof(matchStart2)));

    match_id = matchStart1.match_id;
    ASSERT_EQ(matchStart1.match_id, matchStart2.match_id);
    std::cout << "Match created: match_id=" << match_id << std::endl;

    // Step 4: Both players place ships
    std::cout << "\n=== Step 4: Ship placement ===" << std::endl;

    ShipPlacementMessage placement1;
    memset(&placement1, 0, sizeof(placement1));
    placement1.match_id = match_id;
    placement1.ready = true;

    // Place ships for player 1 (horizontal line at top)
    placement1.ships[0] = Ship{SHIP_CARRIER, {0, 0}, HORIZONTAL, 5, false};
    placement1.ships[1] = Ship{SHIP_BATTLESHIP, {1, 0}, HORIZONTAL, 4, false};
    placement1.ships[2] = Ship{SHIP_CRUISER, {2, 0}, HORIZONTAL, 3, false};
    placement1.ships[3] = Ship{SHIP_SUBMARINE, {3, 0}, HORIZONTAL, 3, false};
    placement1.ships[4] = Ship{SHIP_DESTROYER, {4, 0}, HORIZONTAL, 2, false};

    ASSERT_TRUE(sendMessage(client1_fd, MessageType::SHIP_PLACEMENT, &placement1, sizeof(placement1)));

    ShipPlacementAck ack1;
    ASSERT_TRUE(skipBroadcastMessages(client1_fd, MessageType::SHIP_PLACEMENT_ACK, &ack1, sizeof(ack1)));
    ASSERT_TRUE(ack1.valid) << "Player 1 ship placement invalid: " << ack1.error_message;
    std::cout << "Player 1 ships placed" << std::endl;

    // Place ships for player 2 (vertical line on left)
    ShipPlacementMessage placement2;
    memset(&placement2, 0, sizeof(placement2));
    placement2.match_id = match_id;
    placement2.ready = true;

    placement2.ships[0] = Ship{SHIP_CARRIER, {0, 5}, VERTICAL, 5, false};
    placement2.ships[1] = Ship{SHIP_BATTLESHIP, {0, 6}, VERTICAL, 4, false};
    placement2.ships[2] = Ship{SHIP_CRUISER, {0, 7}, VERTICAL, 3, false};
    placement2.ships[3] = Ship{SHIP_SUBMARINE, {0, 8}, VERTICAL, 3, false};
    placement2.ships[4] = Ship{SHIP_DESTROYER, {0, 9}, VERTICAL, 2, false};

    ASSERT_TRUE(sendMessage(client2_fd, MessageType::SHIP_PLACEMENT, &placement2, sizeof(placement2)));

    ShipPlacementAck ack2;
    ASSERT_TRUE(skipBroadcastMessages(client2_fd, MessageType::SHIP_PLACEMENT_ACK, &ack2, sizeof(ack2)));
    ASSERT_TRUE(ack2.valid) << "Player 2 ship placement invalid: " << ack2.error_message;
    std::cout << "Player 2 ships placed" << std::endl;

    // Both players receive MATCH_READY
    MatchStateMessage matchReady1, matchReady2;
    ASSERT_TRUE(skipBroadcastMessages(client1_fd, MessageType::MATCH_READY, &matchReady1, sizeof(matchReady1)));
    ASSERT_TRUE(skipBroadcastMessages(client2_fd, MessageType::MATCH_READY, &matchReady2, sizeof(matchReady2)));

    uint32_t first_player = matchReady1.current_turn_player_id;
    std::cout << "Match ready! First turn: player " << first_player << std::endl;

    // Step 5: Play a few turns
    std::cout << "\n=== Step 5: Gameplay - Take turns ===" << std::endl;

    // Make a move
    MoveMessage move1;
    memset(&move1, 0, sizeof(move1));
    move1.match_id = match_id;
    move1.target = {0, 5};  // Target player 2's carrier

    int current_player_fd = (first_player == user1_id) ? client1_fd : client2_fd;
    ASSERT_TRUE(sendMessage(current_player_fd, MessageType::MOVE, &move1, sizeof(move1)));
    std::cout << "Player " << first_player << " shoots at (0,5)" << std::endl;

    // Receive MOVE_RESULT
    MoveResultMessage moveResult;
    MessageHeader header;
    ASSERT_TRUE(receiveMessage(current_player_fd, header, &moveResult, sizeof(moveResult)));
    ASSERT_EQ(header.type, MessageType::MOVE_RESULT);
    std::cout << "Result: " << (moveResult.result == SHOT_HIT ? "HIT" :
                                moveResult.result == SHOT_MISS ? "MISS" : "SUNK") << std::endl;

    // Receive TURN_UPDATE
    TurnUpdateMessage turnUpdate;
    ASSERT_TRUE(skipBroadcastMessages(current_player_fd, MessageType::TURN_UPDATE, &turnUpdate, sizeof(turnUpdate)));
    std::cout << "Turn updated. Current turn: player " << turnUpdate.current_player_id << std::endl;

    std::cout << "\n✅ Complete gameplay flow test PASSED!" << std::endl;
}

int main(int argc, char** argv) {
    srand(time(nullptr));
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
