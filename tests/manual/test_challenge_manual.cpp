#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"
#include "messages/authentication_messages.h"
#include "messages/matchmaking_messages.h"
#include "message_serialization.h"

using namespace MessageSerialization;

// Simple client for testing
class SimpleClient {
private:
    int sock_;
    bool connected_;
    uint32_t user_id_;
    char session_token_[64];

public:
    SimpleClient() : sock_(-1), connected_(false), user_id_(0) {
        memset(session_token_, 0, sizeof(session_token_));
    }

    ~SimpleClient() {
        disconnect();
    }

    bool connect(const char* host, int port) {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            return false;
        }

        if (::connect(sock_, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "Connection failed" << std::endl;
            return false;
        }

        connected_ = true;
        std::cout << "[CLIENT] Connected to " << host << ":" << port << std::endl;
        return true;
    }

    void disconnect() {
        if (sock_ >= 0) {
            close(sock_);
            sock_ = -1;
        }
        connected_ = false;
    }

    bool sendMessage(const MessageHeader& header, const std::string& payload) {
        // Send header
        if (send(sock_, &header, sizeof(header), 0) != sizeof(header)) {
            return false;
        }

        // Send payload if any
        if (header.length > 0 && !payload.empty()) {
            if (send(sock_, payload.data(), header.length, 0) != (ssize_t)header.length) {
                return false;
            }
        }

        return true;
    }

    bool receiveMessage(MessageHeader& header, std::string& payload) {
        // Receive header
        ssize_t n = recv(sock_, &header, sizeof(header), 0);
        if (n != sizeof(header)) {
            return false;
        }

        // Receive payload if any
        if (header.length > 0) {
            payload.resize(header.length);
            n = recv(sock_, &payload[0], header.length, 0);
            if (n != (ssize_t)header.length) {
                return false;
            }
        } else {
            payload.clear();
        }

        return true;
    }

    bool registerUser(const std::string& username, const std::string& password, const std::string& display_name) {
        RegisterRequest req;
        safeStrCopy(req.username, username, sizeof(req.username));
        safeStrCopy(req.password, password, sizeof(req.password));
        safeStrCopy(req.display_name, display_name, sizeof(req.display_name));

        MessageHeader header;
        memset(&header, 0, sizeof(header));
        header.type = AUTH_REGISTER;
        header.length = sizeof(req);
        header.timestamp = time(nullptr);

        std::string payload = serialize(req);
        if (!sendMessage(header, payload)) {
            std::cerr << "[CLIENT] Failed to send register request" << std::endl;
            return false;
        }

        // Wait for response
        MessageHeader resp_header;
        std::string resp_payload;
        if (!receiveMessage(resp_header, resp_payload)) {
            std::cerr << "[CLIENT] Failed to receive register response" << std::endl;
            return false;
        }

        RegisterResponse resp;
        if (!deserialize(resp_payload, resp)) {
            std::cerr << "[CLIENT] Failed to deserialize register response" << std::endl;
            return false;
        }

        if (resp.success) {
            std::cout << "[CLIENT] Registered: " << username << " (user_id=" << resp.user_id << ")" << std::endl;
            return true;
        } else {
            std::cerr << "[CLIENT] Register failed: " << resp.error_message << std::endl;
            return false;
        }
    }

    bool login(const std::string& username, const std::string& password) {
        LoginRequest req;
        safeStrCopy(req.username, username, sizeof(req.username));
        safeStrCopy(req.password, password, sizeof(req.password));

        MessageHeader header;
        memset(&header, 0, sizeof(header));
        header.type = AUTH_LOGIN;
        header.length = sizeof(req);
        header.timestamp = time(nullptr);

        std::string payload = serialize(req);
        if (!sendMessage(header, payload)) {
            std::cerr << "[CLIENT] Failed to send login request" << std::endl;
            return false;
        }

        // Wait for response
        MessageHeader resp_header;
        std::string resp_payload;
        if (!receiveMessage(resp_header, resp_payload)) {
            std::cerr << "[CLIENT] Failed to receive login response" << std::endl;
            return false;
        }

        // Skip PLAYER_STATUS_UPDATE if present
        while (resp_header.type == PLAYER_STATUS_UPDATE) {
            std::cout << "[CLIENT] Skipping PLAYER_STATUS_UPDATE broadcast" << std::endl;
            if (!receiveMessage(resp_header, resp_payload)) {
                std::cerr << "[CLIENT] Failed to receive next message" << std::endl;
                return false;
            }
        }

        LoginResponse resp;
        if (!deserialize(resp_payload, resp)) {
            std::cerr << "[CLIENT] Failed to deserialize login response" << std::endl;
            return false;
        }

        if (resp.success) {
            user_id_ = resp.user_id;
            memcpy(session_token_, resp.session_token, sizeof(session_token_));
            std::cout << "[CLIENT] Logged in: " << username << " (user_id=" << user_id_ << ")" << std::endl;
            return true;
        } else {
            std::cerr << "[CLIENT] Login failed: " << resp.error_message << std::endl;
            return false;
        }
    }

    bool sendChallenge(uint32_t target_user_id) {
        ChallengeRequest req;
        req.target_user_id = target_user_id;
        req.time_limit = 60;
        req.random_placement = false;

        MessageHeader header;
        memset(&header, 0, sizeof(header));
        header.type = CHALLENGE_SEND;
        header.length = sizeof(req);
        header.timestamp = time(nullptr);
        memcpy(header.session_token, session_token_, sizeof(session_token_));

        std::string payload = serialize(req);
        if (!sendMessage(header, payload)) {
            std::cerr << "[CLIENT] Failed to send challenge" << std::endl;
            return false;
        }

        std::cout << "[CLIENT] Challenge sent to user_id=" << target_user_id << std::endl;
        return true;
    }

    bool receiveChallenge(uint32_t& challenge_id, uint32_t& challenger_id) {
        MessageHeader header;
        std::string payload;

        // Skip any status updates
        while (true) {
            if (!receiveMessage(header, payload)) {
                std::cerr << "[CLIENT] Failed to receive message" << std::endl;
                return false;
            }

            if (header.type == CHALLENGE_RECEIVED) {
                break;
            } else if (header.type == PLAYER_STATUS_UPDATE) {
                std::cout << "[CLIENT] Skipping PLAYER_STATUS_UPDATE" << std::endl;
                continue;
            } else {
                std::cerr << "[CLIENT] Expected CHALLENGE_RECEIVED, got type=" << (int)header.type << std::endl;
                return false;
            }
        }

        ChallengeReceived msg;
        if (!deserialize(payload, msg)) {
            std::cerr << "[CLIENT] Failed to deserialize challenge" << std::endl;
            return false;
        }

        challenge_id = msg.challenge_id;
        challenger_id = msg.challenger_id;

        std::cout << "[CLIENT] Received challenge #" << challenge_id
                  << " from " << msg.challenger_name
                  << " (ELO: " << msg.challenger_elo << ")" << std::endl;

        return true;
    }

    bool respondChallenge(uint32_t challenge_id, bool accept) {
        ChallengeResponse resp;
        resp.challenge_id = challenge_id;
        resp.accepted = accept;

        MessageHeader header;
        memset(&header, 0, sizeof(header));
        header.type = CHALLENGE_RESPONSE;
        header.length = sizeof(resp);
        header.timestamp = time(nullptr);
        memcpy(header.session_token, session_token_, sizeof(session_token_));

        std::string payload = serialize(resp);
        if (!sendMessage(header, payload)) {
            std::cerr << "[CLIENT] Failed to send challenge response" << std::endl;
            return false;
        }

        std::cout << "[CLIENT] Challenge " << (accept ? "ACCEPTED" : "DECLINED") << std::endl;
        return true;
    }

    bool receiveMatchStart(uint32_t& match_id) {
        MessageHeader header;
        std::string payload;

        // Skip any status updates
        while (true) {
            if (!receiveMessage(header, payload)) {
                std::cerr << "[CLIENT] Failed to receive message" << std::endl;
                return false;
            }

            if (header.type == MATCH_START) {
                break;
            } else if (header.type == PLAYER_STATUS_UPDATE) {
                std::cout << "[CLIENT] Skipping PLAYER_STATUS_UPDATE" << std::endl;
                continue;
            } else {
                std::cerr << "[CLIENT] Unexpected message type: " << (int)header.type << std::endl;
                return false;
            }
        }

        MatchStartMessage msg;
        if (!deserialize(payload, msg)) {
            std::cerr << "[CLIENT] Failed to deserialize match start" << std::endl;
            return false;
        }

        match_id = msg.match_id;

        std::cout << "[CLIENT] Match started! match_id=" << match_id
                  << ", opponent=" << msg.opponent_name
                  << ", you_go_first=" << msg.you_go_first << std::endl;

        return true;
    }

    uint32_t getUserId() const { return user_id_; }
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  CHALLENGE SYSTEM MANUAL TEST" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Create two clients
    SimpleClient client1, client2;

    // Connect both clients
    std::cout << "Step 1: Connecting clients..." << std::endl;
    if (!client1.connect("127.0.0.1", 9999)) {
        std::cerr << "Failed to connect client 1" << std::endl;
        return 1;
    }
    if (!client2.connect("127.0.0.1", 9999)) {
        std::cerr << "Failed to connect client 2" << std::endl;
        return 1;
    }
    std::cout << std::endl;

    // Register and login users
    std::cout << "Step 2: Register and login users..." << std::endl;
    std::string user1 = "challenger_" + std::to_string(time(nullptr));
    std::string user2 = "target_" + std::to_string(time(nullptr));

    if (!client1.registerUser(user1, "pass123", "Challenger User")) {
        return 1;
    }
    if (!client2.registerUser(user2, "pass123", "Target User")) {
        return 1;
    }

    if (!client1.login(user1, "pass123")) {
        return 1;
    }
    if (!client2.login(user2, "pass123")) {
        return 1;
    }

    uint32_t user1_id = client1.getUserId();
    uint32_t user2_id = client2.getUserId();
    std::cout << "User 1 ID: " << user1_id << std::endl;
    std::cout << "User 2 ID: " << user2_id << std::endl;
    std::cout << std::endl;

    // Send challenge
    std::cout << "Step 3: User 1 sends challenge to User 2..." << std::endl;
    if (!client1.sendChallenge(user2_id)) {
        return 1;
    }
    sleep(1);
    std::cout << std::endl;

    // Receive challenge
    std::cout << "Step 4: User 2 receives challenge..." << std::endl;
    uint32_t challenge_id, challenger_id;
    if (!client2.receiveChallenge(challenge_id, challenger_id)) {
        return 1;
    }
    std::cout << std::endl;

    // Accept challenge
    std::cout << "Step 5: User 2 accepts challenge..." << std::endl;
    if (!client2.respondChallenge(challenge_id, true)) {
        return 1;
    }
    sleep(1);
    std::cout << std::endl;

    // Both clients receive match start
    std::cout << "Step 6: Both clients receive MATCH_START..." << std::endl;
    uint32_t match_id1, match_id2;
    if (!client1.receiveMatchStart(match_id1)) {
        std::cerr << "Client 1 failed to receive match start" << std::endl;
        return 1;
    }
    if (!client2.receiveMatchStart(match_id2)) {
        std::cerr << "Client 2 failed to receive match start" << std::endl;
        return 1;
    }

    if (match_id1 != match_id2) {
        std::cerr << "Match ID mismatch! client1=" << match_id1 << ", client2=" << match_id2 << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  ✅ CHALLENGE SYSTEM TEST PASSED!" << std::endl;
    std::cout << "  Match created: #" << match_id1 << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
