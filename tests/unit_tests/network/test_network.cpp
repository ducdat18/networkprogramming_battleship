#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "protocol.h"
#include "client_connection.h"

// Helper: Create a simple client socket
int createClientSocket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

// ============== PROTOCOL TESTS ==============

TEST(ProtocolTest, MessageHeaderSize) {
    // Verify MessageHeader size for protocol compatibility
    EXPECT_EQ(sizeof(MessageHeader), 77);  // 1 + 4 + 8 + 64
}

TEST(ProtocolTest, MessageTypes) {
    // Verify important message type values
    EXPECT_EQ(AUTH_REGISTER, 1);
    EXPECT_EQ(AUTH_LOGIN, 2);
    EXPECT_EQ(PING, 101);
    EXPECT_EQ(PONG, 102);
}

TEST(ProtocolTest, ShipLength) {
    EXPECT_EQ(getShipLength(SHIP_CARRIER), 5);
    EXPECT_EQ(getShipLength(SHIP_BATTLESHIP), 4);
    EXPECT_EQ(getShipLength(SHIP_CRUISER), 3);
    EXPECT_EQ(getShipLength(SHIP_SUBMARINE), 3);
    EXPECT_EQ(getShipLength(SHIP_DESTROYER), 2);
}

TEST(ProtocolTest, MessageTypeToString) {
    std::string ping_str = messageTypeToString(PING);
    EXPECT_FALSE(ping_str.empty());

    std::string login_str = messageTypeToString(AUTH_LOGIN);
    EXPECT_FALSE(login_str.empty());
}

// ============== CLIENT CONNECTION TESTS ==============

class ClientConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a socket pair for testing
        int sockets[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0) {
            client_fd = sockets[0];
            test_fd = sockets[1];
            connection = new ClientConnection(client_fd);
        } else {
            client_fd = -1;
            test_fd = -1;
            connection = nullptr;
        }
    }

    void TearDown() override {
        if (connection) {
            delete connection;
        }
        if (test_fd >= 0) {
            close(test_fd);
        }
        // client_fd closed by ClientConnection destructor
    }

    ClientConnection* connection;
    int client_fd;
    int test_fd;
};

TEST_F(ClientConnectionTest, InitialState) {
    ASSERT_NE(connection, nullptr);
    EXPECT_TRUE(connection->isConnected());
    EXPECT_FALSE(connection->isAuthenticated());
    EXPECT_EQ(connection->getUserId(), 0);
    EXPECT_EQ(connection->getBytesSent(), 0);
    EXPECT_EQ(connection->getBytesReceived(), 0);
}

TEST_F(ClientConnectionTest, SendMessage_EmptyPayload) {
    ASSERT_NE(connection, nullptr);

    MessageHeader header;
    header.type = PING;
    header.length = 0;
    header.timestamp = time(nullptr);
    memset(header.session_token, 0, sizeof(header.session_token));

    bool sent = connection->sendMessage(header, "");
    EXPECT_TRUE(sent);
    EXPECT_GT(connection->getBytesSent(), 0);
}

TEST_F(ClientConnectionTest, SendMessage_WithPayload) {
    ASSERT_NE(connection, nullptr);

    MessageHeader header;
    header.type = AUTH_LOGIN;
    header.length = 10;
    header.timestamp = time(nullptr);
    memset(header.session_token, 0, sizeof(header.session_token));

    std::string payload = "testpayload";  // 11 chars but we'll send 10
    payload.resize(10);

    bool sent = connection->sendMessage(header, payload);
    EXPECT_TRUE(sent);
    EXPECT_EQ(connection->getBytesSent(), sizeof(MessageHeader) + 10);
}

TEST_F(ClientConnectionTest, ReceiveMessage_EmptyPayload) {
    ASSERT_NE(connection, nullptr);

    // Send a message from test_fd
    MessageHeader send_header;
    send_header.type = PONG;
    send_header.length = 0;
    send_header.timestamp = time(nullptr);
    memset(send_header.session_token, 0, sizeof(send_header.session_token));

    ssize_t written = write(test_fd, &send_header, sizeof(MessageHeader));
    ASSERT_EQ(written, sizeof(MessageHeader));

    // Receive message
    MessageHeader recv_header;
    std::string payload;
    bool received = connection->receiveMessage(recv_header, payload);

    EXPECT_TRUE(received);
    EXPECT_EQ(recv_header.type, PONG);
    EXPECT_EQ(recv_header.length, 0);
    EXPECT_TRUE(payload.empty());
    EXPECT_GT(connection->getBytesReceived(), 0);
}

TEST_F(ClientConnectionTest, ReceiveMessage_WithPayload) {
    ASSERT_NE(connection, nullptr);

    // Send a message from test_fd
    MessageHeader send_header;
    send_header.type = AUTH_RESPONSE;
    send_header.length = 5;
    send_header.timestamp = time(nullptr);
    memset(send_header.session_token, 0, sizeof(send_header.session_token));

    std::string test_payload = "hello";

    write(test_fd, &send_header, sizeof(MessageHeader));
    write(test_fd, test_payload.data(), test_payload.size());

    // Receive message
    MessageHeader recv_header;
    std::string recv_payload;
    bool received = connection->receiveMessage(recv_header, recv_payload);

    EXPECT_TRUE(received);
    EXPECT_EQ(recv_header.type, AUTH_RESPONSE);
    EXPECT_EQ(recv_header.length, 5);
    EXPECT_EQ(recv_payload, "hello");
}

TEST_F(ClientConnectionTest, Authentication) {
    ASSERT_NE(connection, nullptr);

    EXPECT_FALSE(connection->isAuthenticated());

    connection->setAuthenticated(123, "test_token_abc");

    EXPECT_TRUE(connection->isAuthenticated());
    EXPECT_EQ(connection->getUserId(), 123);
    EXPECT_EQ(connection->getSessionToken(), "test_token_abc");
}

TEST_F(ClientConnectionTest, Disconnect) {
    ASSERT_NE(connection, nullptr);

    EXPECT_TRUE(connection->isConnected());

    connection->disconnect();

    EXPECT_FALSE(connection->isConnected());

    // Try to send after disconnect - should fail
    MessageHeader header;
    header.type = PING;
    header.length = 0;
    header.timestamp = time(nullptr);

    bool sent = connection->sendMessage(header, "");
    EXPECT_FALSE(sent);
}

// ============== INTEGRATION TESTS ==============
// These tests require the server to be running

class NetworkIntegrationTest : public ::testing::Test {
protected:
    static const int TEST_PORT = 9999;  // Different from production port

    void SetUp() override {
        // We'll skip these if server is not available
        // For now, just test protocol and connection logic
    }
};

TEST_F(NetworkIntegrationTest, DISABLED_ConnectToServer) {
    // This test requires server running on TEST_PORT
    // Disabled by default, enable with --gtest_also_run_disabled_tests

    int sock = createClientSocket(TEST_PORT);
    EXPECT_GE(sock, 0);

    if (sock >= 0) {
        close(sock);
    }
}

TEST_F(NetworkIntegrationTest, DISABLED_PingPong) {
    // This test requires server running on TEST_PORT

    int sock = createClientSocket(TEST_PORT);
    ASSERT_GE(sock, 0);

    // Send PING
    MessageHeader ping_header;
    ping_header.type = PING;
    ping_header.length = 0;
    ping_header.timestamp = time(nullptr);
    memset(ping_header.session_token, 0, sizeof(ping_header.session_token));

    ssize_t sent = send(sock, &ping_header, sizeof(MessageHeader), 0);
    EXPECT_EQ(sent, sizeof(MessageHeader));

    // Receive PONG
    MessageHeader pong_header;
    ssize_t received = recv(sock, &pong_header, sizeof(MessageHeader), 0);
    EXPECT_EQ(received, sizeof(MessageHeader));
    EXPECT_EQ(pong_header.type, PONG);

    close(sock);
}

// Main function
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
