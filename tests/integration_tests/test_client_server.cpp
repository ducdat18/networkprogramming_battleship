#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "protocol.h"
#include "config.h"

/**
 * Integration Tests - Test real client-server communication
 *
 * REQUIREMENTS:
 * - Server must be running on localhost (port defined in config.h)
 * - Run server: ./bin/battleship_server
 *
 * These tests connect to the actual server and test real network communication
 */

// Helper class for client connection
class TestClient {
public:
    TestClient() : socket_fd_(-1), connected_(false) {}

    ~TestClient() {
        disconnect();
    }

    bool connect(const char* host, int port) {
        socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            return false;
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }

        if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }

        connected_ = true;
        return true;
    }

    void disconnect() {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
        connected_ = false;
    }

    bool sendMessage(const MessageHeader& header, const std::string& payload) {
        if (!connected_) return false;

        // Send header
        if (!sendData(&header, sizeof(MessageHeader))) {
            return false;
        }

        // Send payload if present
        if (header.length > 0 && !payload.empty()) {
            if (!sendData(payload.data(), header.length)) {
                return false;
            }
        }

        return true;
    }

    bool receiveMessage(MessageHeader& header, std::string& payload, int timeout_ms = 5000) {
        if (!connected_) return false;

        // Set receive timeout
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Receive header
        if (!receiveData(&header, sizeof(MessageHeader))) {
            return false;
        }

        // Validate header
        if (header.length > MAX_MESSAGE_SIZE) {
            return false;
        }

        // Receive payload if present
        payload.clear();
        if (header.length > 0) {
            payload.resize(header.length);
            if (!receiveData(&payload[0], header.length)) {
                return false;
            }
        }

        return true;
    }

    bool isConnected() const { return connected_; }

private:
    bool sendData(const void* data, size_t size) {
        size_t total_sent = 0;
        const char* buffer = static_cast<const char*>(data);

        while (total_sent < size) {
            ssize_t sent = send(socket_fd_, buffer + total_sent, size - total_sent, 0);
            if (sent <= 0) {
                return false;
            }
            total_sent += sent;
        }
        return true;
    }

    bool receiveData(void* data, size_t size) {
        size_t total_received = 0;
        char* buffer = static_cast<char*>(data);

        while (total_received < size) {
            ssize_t received = recv(socket_fd_, buffer + total_received, size - total_received, 0);
            if (received <= 0) {
                return false;
            }
            total_received += received;
        }
        return true;
    }

    int socket_fd_;
    bool connected_;
};

// Test fixture for integration tests
class IntegrationTest : public ::testing::Test {
protected:
    static const char* TEST_SERVER_HOST;
    static const int TEST_SERVER_PORT = SERVER_PORT;  // Use config.h

    void SetUp() override {
        client = new TestClient();
    }

    void TearDown() override {
        if (client) {
            delete client;
            client = nullptr;
        }
    }

    TestClient* client;
};

const char* IntegrationTest::TEST_SERVER_HOST = "127.0.0.1";  // Same as config.h SERVER_HOST

// ============== CONNECTION TESTS ==============

TEST_F(IntegrationTest, ConnectToServer) {
    bool connected = client->connect(TEST_SERVER_HOST, TEST_SERVER_PORT);

    ASSERT_TRUE(connected)
        << "Failed to connect to server at " << TEST_SERVER_HOST << ":" << SERVER_PORT
        << "\nMake sure server is running: ./bin/battleship_server";

    EXPECT_TRUE(client->isConnected());
}

TEST_F(IntegrationTest, ConnectAndDisconnect) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, TEST_SERVER_PORT));
    EXPECT_TRUE(client->isConnected());

    client->disconnect();
    EXPECT_FALSE(client->isConnected());
}

TEST_F(IntegrationTest, MultipleConnections) {
    // Test multiple clients can connect simultaneously
    TestClient client1, client2, client3;

    EXPECT_TRUE(client1.connect(TEST_SERVER_HOST, TEST_SERVER_PORT));
    EXPECT_TRUE(client2.connect(TEST_SERVER_HOST, TEST_SERVER_PORT));
    EXPECT_TRUE(client3.connect(TEST_SERVER_HOST, TEST_SERVER_PORT));

    // All should be connected
    EXPECT_TRUE(client1.isConnected());
    EXPECT_TRUE(client2.isConnected());
    EXPECT_TRUE(client3.isConnected());
}

// ============== PING/PONG TESTS ==============

TEST_F(IntegrationTest, PingPong_Single) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, TEST_SERVER_PORT));

    // Send PING
    MessageHeader ping_header;
    ping_header.type = PING;
    ping_header.length = 0;
    ping_header.timestamp = time(nullptr);
    memset(ping_header.session_token, 0, sizeof(ping_header.session_token));

    ASSERT_TRUE(client->sendMessage(ping_header, ""))
        << "Failed to send PING message";

    // Receive PONG
    MessageHeader pong_header;
    std::string payload;
    ASSERT_TRUE(client->receiveMessage(pong_header, payload))
        << "Failed to receive PONG response";

    // Verify PONG
    EXPECT_EQ(pong_header.type, PONG)
        << "Expected PONG (" << PONG << "), got " << (int)pong_header.type;
    EXPECT_EQ(pong_header.length, 0);
    EXPECT_TRUE(payload.empty());
}

TEST_F(IntegrationTest, PingPong_Multiple) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, TEST_SERVER_PORT));

    // Send 10 PINGs and verify 10 PONGs
    for (int i = 0; i < 10; i++) {
        // Send PING
        MessageHeader ping_header;
        ping_header.type = PING;
        ping_header.length = 0;
        ping_header.timestamp = time(nullptr);
        memset(ping_header.session_token, 0, sizeof(ping_header.session_token));

        ASSERT_TRUE(client->sendMessage(ping_header, ""))
            << "Failed to send PING #" << i;

        // Receive PONG
        MessageHeader pong_header;
        std::string payload;
        ASSERT_TRUE(client->receiveMessage(pong_header, payload))
            << "Failed to receive PONG #" << i;

        EXPECT_EQ(pong_header.type, PONG)
            << "PING #" << i << " - Expected PONG, got " << (int)pong_header.type;
    }
}

TEST_F(IntegrationTest, PingPong_Concurrent) {
    // Multiple clients sending PING simultaneously
    const int NUM_CLIENTS = 5;
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);

    for (int i = 0; i < NUM_CLIENTS; i++) {
        threads.emplace_back([&success_count]() {
            TestClient test_client;

            // Retry connection a few times
            int retries = 3;
            while (retries-- > 0) {
                if (test_client.connect(IntegrationTest::TEST_SERVER_HOST, IntegrationTest::TEST_SERVER_PORT)) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            if (!test_client.isConnected()) {
                return;
            }

            // Send PING
            MessageHeader ping_header;
            ping_header.type = PING;
            ping_header.length = 0;
            ping_header.timestamp = time(nullptr);
            memset(ping_header.session_token, 0, sizeof(ping_header.session_token));

            if (!test_client.sendMessage(ping_header, "")) {
                return;
            }

            // Receive PONG
            MessageHeader pong_header;
            std::string payload;
            if (!test_client.receiveMessage(pong_header, payload)) {
                return;
            }

            if (pong_header.type == PONG) {
                success_count++;
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count, NUM_CLIENTS)
        << "Not all concurrent clients received PONG";
}

// ============== STRESS TESTS ==============

TEST_F(IntegrationTest, StressTest_RapidPingPong) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, TEST_SERVER_PORT));

    const int NUM_MESSAGES = 100;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_MESSAGES; i++) {
        // Send PING
        MessageHeader ping_header;
        ping_header.type = PING;
        ping_header.length = 0;
        ping_header.timestamp = time(nullptr);
        memset(ping_header.session_token, 0, sizeof(ping_header.session_token));

        ASSERT_TRUE(client->sendMessage(ping_header, ""));

        // Receive PONG
        MessageHeader pong_header;
        std::string payload;
        ASSERT_TRUE(client->receiveMessage(pong_header, payload));
        ASSERT_EQ(pong_header.type, PONG);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "[PERFORMANCE] " << NUM_MESSAGES << " PING/PONG in "
              << duration.count() << "ms ("
              << (float)duration.count() / NUM_MESSAGES << "ms per message)" << std::endl;

    // Should be able to do 100 messages in under 1 second
    EXPECT_LT(duration.count(), 1000)
        << "Performance too slow: " << duration.count() << "ms for " << NUM_MESSAGES << " messages";
}

// ============== ERROR HANDLING TESTS ==============

TEST_F(IntegrationTest, ConnectToInvalidPort) {
    bool connected = client->connect(TEST_SERVER_HOST, 1); // Invalid port (requires root)
    EXPECT_FALSE(connected);
}

TEST_F(IntegrationTest, SendAfterDisconnect) {
    ASSERT_TRUE(client->connect(TEST_SERVER_HOST, TEST_SERVER_PORT));
    client->disconnect();

    MessageHeader header;
    header.type = PING;
    header.length = 0;

    bool sent = client->sendMessage(header, "");
    EXPECT_FALSE(sent);
}

// Main function
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);

    std::cout << "========================================" << std::endl;
    std::cout << "  INTEGRATION TESTS" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "REQUIREMENTS:" << std::endl;
    std::cout << "  1. Server must be running: ./bin/battleship_server" << std::endl;
    std::cout << "  2. Server port: " << SERVER_PORT << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Starting tests..." << std::endl;
    std::cout << "" << std::endl;

    return RUN_ALL_TESTS();
}
