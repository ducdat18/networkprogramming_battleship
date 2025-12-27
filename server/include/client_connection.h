#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <string>
#include <cstdint>
#include <memory>
#include <atomic>
#include "protocol.h"

/**
 * Represents a single client connection
 * Handles reading/writing messages for one client
 */
class ClientConnection {
public:
    ClientConnection(int socket_fd);
    ~ClientConnection();

    // Message I/O
    bool sendMessage(const MessageHeader& header, const std::string& payload);
    bool receiveMessage(MessageHeader& header, std::string& payload);

    // Connection info
    int getSocketFd() const { return socket_fd_; }
    uint32_t getUserId() const { return user_id_; }
    std::string getSessionToken() const { return session_token_; }
    bool isAuthenticated() const { return authenticated_; }

    // State management
    void setAuthenticated(uint32_t user_id, const std::string& token);
    void disconnect();
    bool isConnected() const { return connected_; }

    // Statistics
    uint64_t getBytesSent() const { return bytes_sent_; }
    uint64_t getBytesReceived() const { return bytes_received_; }

private:
    // Socket operations
    bool sendData(const void* data, size_t size);
    bool receiveData(void* data, size_t size);

    // Connection details
    int socket_fd_;
    std::atomic<bool> connected_;
    std::atomic<bool> authenticated_;

    // User info
    uint32_t user_id_;
    std::string session_token_;

    // Statistics
    std::atomic<uint64_t> bytes_sent_;
    std::atomic<uint64_t> bytes_received_;
};

#endif // CLIENT_CONNECTION_H
