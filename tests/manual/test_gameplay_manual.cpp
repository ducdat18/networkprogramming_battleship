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

int connectToServer() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_HOST, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to server" << std::endl;
        close(sockfd);
        return -1;
    }

    std::cout << "✅ Connected to server" << std::endl;
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

bool skipBroadcastMessages(int sockfd, MessageType expected_type, void* payload, size_t max_payload_size) {
    MessageHeader header;
    for (int i = 0; i < 20; i++) {
        if (!receiveMessage(sockfd, header, payload, max_payload_size)) {
            return false;
        }

        if (header.type == expected_type) {
            return true;
        }

        if (header.type == MessageType::PLAYER_STATUS_UPDATE) {
            continue;
        }

        std::cerr << "⚠️  Unexpected message type: " << header.type << " (expected: " << expected_type << ")" << std::endl;
    }
    return false;
}

int main() {
    std::cout << "\n╔══════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🎮 BATTLESHIP GAMEPLAY MANUAL TEST 🎮  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════╝\n" << std::endl;

    srand(time(nullptr));

    // Connect both clients
    std::cout << "🔌 Connecting clients..." << std::endl;
    int client1_fd = connectToServer();
    if (client1_fd < 0) return 1;

    int client2_fd = connectToServer();
    if (client2_fd < 0) {
        close(client1_fd);
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Register and login
    std::cout << "\n👤 Registering and logging in players..." << std::endl;

    long timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    std::string username1 = "test1_" + std::to_string(timestamp);
    std::string username2 = "test2_" + std::to_string(timestamp);

    // Player 1 register
    RegisterRequest reg1;
    memset(&reg1, 0, sizeof(reg1));
    strncpy(reg1.username, username1.c_str(), sizeof(reg1.username) - 1);
    strncpy(reg1.password, "pass123", sizeof(reg1.password) - 1);
    strncpy(reg1.display_name, "Test Player 1", sizeof(reg1.display_name) - 1);

    sendMessage(client1_fd, MessageType::REGISTER, &reg1, sizeof(reg1));

    RegisterResponse regResp1;
    if (!skipBroadcastMessages(client1_fd, MessageType::REGISTER_RESPONSE, &regResp1, sizeof(regResp1)) || !regResp1.success) {
        std::cerr << "❌ Player 1 registration failed" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    uint32_t user1_id = regResp1.user_id;
    std::cout << "  ✅ Player 1 registered: user_id=" << user1_id << std::endl;

    // Player 1 login
    LoginRequest login1;
    memset(&login1, 0, sizeof(login1));
    strncpy(login1.username, username1.c_str(), sizeof(login1.username) - 1);
    strncpy(login1.password, "pass123", sizeof(login1.password) - 1);

    sendMessage(client1_fd, MessageType::LOGIN, &login1, sizeof(login1));

    LoginResponse loginResp1;
    if (!skipBroadcastMessages(client1_fd, MessageType::LOGIN_RESPONSE, &loginResp1, sizeof(loginResp1)) || !loginResp1.success) {
        std::cerr << "❌ Player 1 login failed" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    std::cout << "  ✅ Player 1 logged in" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Player 2 register
    RegisterRequest reg2;
    memset(&reg2, 0, sizeof(reg2));
    strncpy(reg2.username, username2.c_str(), sizeof(reg2.username) - 1);
    strncpy(reg2.password, "pass456", sizeof(reg2.password) - 1);
    strncpy(reg2.display_name, "Test Player 2", sizeof(reg2.display_name) - 1);

    sendMessage(client2_fd, MessageType::REGISTER, &reg2, sizeof(reg2));

    RegisterResponse regResp2;
    if (!skipBroadcastMessages(client2_fd, MessageType::REGISTER_RESPONSE, &regResp2, sizeof(regResp2)) || !regResp2.success) {
        std::cerr << "❌ Player 2 registration failed" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    uint32_t user2_id = regResp2.user_id;
    std::cout << "  ✅ Player 2 registered: user_id=" << user2_id << std::endl;

    // Player 2 login
    LoginRequest login2;
    memset(&login2, 0, sizeof(login2));
    strncpy(login2.username, username2.c_str(), sizeof(login2.username) - 1);
    strncpy(login2.password, "pass456", sizeof(login2.password) - 1);

    sendMessage(client2_fd, MessageType::LOGIN, &login2, sizeof(login2));

    LoginResponse loginResp2;
    if (!skipBroadcastMessages(client2_fd, MessageType::LOGIN_RESPONSE, &loginResp2, sizeof(loginResp2)) || !loginResp2.success) {
        std::cerr << "❌ Player 2 login failed" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    std::cout << "  ✅ Player 2 logged in" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Send challenge
    std::cout << "\n⚔️  Sending challenge..." << std::endl;

    ChallengeSend challenge;
    memset(&challenge, 0, sizeof(challenge));
    challenge.target_user_id = user2_id;
    challenge.time_limit = 60;
    challenge.random_placement = false;

    sendMessage(client1_fd, MessageType::CHALLENGE_SEND, &challenge, sizeof(challenge));

    ChallengeReceived challengeRecv;
    if (!skipBroadcastMessages(client2_fd, MessageType::CHALLENGE_RECEIVED, &challengeRecv, sizeof(challengeRecv))) {
        std::cerr << "❌ Failed to receive challenge" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    std::cout << "  ✅ Challenge received: challenge_id=" << challengeRecv.challenge_id << std::endl;

    // Accept challenge
    std::cout << "\n✅ Accepting challenge..." << std::endl;

    ChallengeResponse response;
    memset(&response, 0, sizeof(response));
    response.challenge_id = challengeRecv.challenge_id;
    response.accept = true;

    sendMessage(client2_fd, MessageType::CHALLENGE_RESPONSE, &response, sizeof(response));

    MatchStartMessage matchStart1, matchStart2;
    if (!skipBroadcastMessages(client1_fd, MessageType::MATCH_START, &matchStart1, sizeof(matchStart1))) {
        std::cerr << "❌ Player 1 didn't receive MATCH_START" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    if (!skipBroadcastMessages(client2_fd, MessageType::MATCH_START, &matchStart2, sizeof(matchStart2))) {
        std::cerr << "❌ Player 2 didn't receive MATCH_START" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }

    uint32_t match_id = matchStart1.match_id;
    std::cout << "  🎮 Match created: match_id=" << match_id << std::endl;

    // Ship placement
    std::cout << "\n🚢 Placing ships..." << std::endl;

    ShipPlacementMessage placement1;
    memset(&placement1, 0, sizeof(placement1));
    placement1.match_id = match_id;
    placement1.ready = true;
    placement1.ships[0] = Ship{SHIP_CARRIER, {0, 0}, HORIZONTAL, 5, false};
    placement1.ships[1] = Ship{SHIP_BATTLESHIP, {1, 0}, HORIZONTAL, 4, false};
    placement1.ships[2] = Ship{SHIP_CRUISER, {2, 0}, HORIZONTAL, 3, false};
    placement1.ships[3] = Ship{SHIP_SUBMARINE, {3, 0}, HORIZONTAL, 3, false};
    placement1.ships[4] = Ship{SHIP_DESTROYER, {4, 0}, HORIZONTAL, 2, false};

    sendMessage(client1_fd, MessageType::SHIP_PLACEMENT, &placement1, sizeof(placement1));

    ShipPlacementAck ack1;
    if (!skipBroadcastMessages(client1_fd, MessageType::SHIP_PLACEMENT_ACK, &ack1, sizeof(ack1)) || !ack1.valid) {
        std::cerr << "❌ Player 1 ship placement invalid: " << ack1.error_message << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    std::cout << "  ✅ Player 1 ships placed" << std::endl;

    ShipPlacementMessage placement2;
    memset(&placement2, 0, sizeof(placement2));
    placement2.match_id = match_id;
    placement2.ready = true;
    placement2.ships[0] = Ship{SHIP_CARRIER, {0, 5}, VERTICAL, 5, false};
    placement2.ships[1] = Ship{SHIP_BATTLESHIP, {0, 6}, VERTICAL, 4, false};
    placement2.ships[2] = Ship{SHIP_CRUISER, {0, 7}, VERTICAL, 3, false};
    placement2.ships[3] = Ship{SHIP_SUBMARINE, {0, 8}, VERTICAL, 3, false};
    placement2.ships[4] = Ship{SHIP_DESTROYER, {0, 9}, VERTICAL, 2, false};

    sendMessage(client2_fd, MessageType::SHIP_PLACEMENT, &placement2, sizeof(placement2));

    ShipPlacementAck ack2;
    if (!skipBroadcastMessages(client2_fd, MessageType::SHIP_PLACEMENT_ACK, &ack2, sizeof(ack2)) || !ack2.valid) {
        std::cerr << "❌ Player 2 ship placement invalid: " << ack2.error_message << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    std::cout << "  ✅ Player 2 ships placed" << std::endl;

    // Receive MATCH_READY
    MatchStateMessage matchReady1, matchReady2;
    if (!skipBroadcastMessages(client1_fd, MessageType::MATCH_READY, &matchReady1, sizeof(matchReady1))) {
        std::cerr << "❌ Player 1 didn't receive MATCH_READY" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    if (!skipBroadcastMessages(client2_fd, MessageType::MATCH_READY, &matchReady2, sizeof(matchReady2))) {
        std::cerr << "❌ Player 2 didn't receive MATCH_READY" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }

    uint32_t first_player = matchReady1.current_turn_player_id;
    std::cout << "  🎮 Match ready! First turn: player " << first_player << std::endl;

    // Make a move
    std::cout << "\n🎯 Making move..." << std::endl;

    MoveMessage move1;
    memset(&move1, 0, sizeof(move1));
    move1.match_id = match_id;
    move1.target = {0, 5};  // Target player 2's carrier

    int current_player_fd = (first_player == user1_id) ? client1_fd : client2_fd;
    sendMessage(current_player_fd, MessageType::MOVE, &move1, sizeof(move1));
    std::cout << "  📍 Player " << first_player << " shoots at (0,5)" << std::endl;

    // Receive MOVE_RESULT
    MessageHeader header;
    MoveResultMessage moveResult;
    if (!receiveMessage(current_player_fd, header, &moveResult, sizeof(moveResult)) || header.type != MessageType::MOVE_RESULT) {
        std::cerr << "❌ Failed to receive MOVE_RESULT" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }

    std::cout << "  💥 Result: " << (moveResult.result == SHOT_HIT ? "HIT" :
                                    moveResult.result == SHOT_MISS ? "MISS" : "SUNK") << std::endl;

    // Receive TURN_UPDATE
    TurnUpdateMessage turnUpdate;
    if (!skipBroadcastMessages(current_player_fd, MessageType::TURN_UPDATE, &turnUpdate, sizeof(turnUpdate))) {
        std::cerr << "❌ Failed to receive TURN_UPDATE" << std::endl;
        close(client1_fd);
        close(client2_fd);
        return 1;
    }
    std::cout << "  🔄 Turn updated. Current turn: player " << turnUpdate.current_player_id << std::endl;

    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║  ✅ ALL GAMEPLAY TESTS PASSED! ✅     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝\n" << std::endl;

    close(client1_fd);
    close(client2_fd);
    return 0;
}
