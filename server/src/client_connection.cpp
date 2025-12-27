#include "client_connection.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

ClientConnection::ClientConnection(int socket_fd)
    : socket_fd_(socket_fd)
    , connected_(true)
    , authenticated_(false)
    , user_id_(0)
    , bytes_sent_(0)
    , bytes_received_(0)
{
}

ClientConnection::~ClientConnection() {
    disconnect();
}

bool ClientConnection::sendMessage(const MessageHeader& header, const std::string& payload) {
    if (!connected_) {
        return false;
    }

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

bool ClientConnection::receiveMessage(MessageHeader& header, std::string& payload) {
    if (!connected_) {
        return false;
    }

    // Receive header
    if (!receiveData(&header, sizeof(MessageHeader))) {
        return false;
    }

    // Validate header
    if (header.length > MAX_MESSAGE_SIZE) {
        std::cerr << "[ERROR] Message too large: " << header.length << " bytes" << std::endl;
        disconnect();
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

bool ClientConnection::sendData(const void* data, size_t size) {
    if (!connected_ || socket_fd_ < 0) {
        return false;
    }

    size_t total_sent = 0;
    const char* buffer = static_cast<const char*>(data);

    while (total_sent < size) {
        ssize_t sent = send(socket_fd_, buffer + total_sent, size - total_sent, MSG_NOSIGNAL);

        if (sent < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted, try again
            }

            // In unit tests we may use dummy file descriptors; avoid noisy errors for EBADF
            if (errno == EBADF) {
                return false;
            }

            std::cerr << "[ERROR] Send failed: " << strerror(errno) << std::endl;
            disconnect();
            return false;
        }

        if (sent == 0) {
            std::cerr << "[WARNING] Send returned 0" << std::endl;
            disconnect();
            return false;
        }

        total_sent += sent;
        bytes_sent_ += sent;
    }

    return true;
}

bool ClientConnection::receiveData(void* data, size_t size) {
    if (!connected_ || socket_fd_ < 0) {
        return false;
    }

    size_t total_received = 0;
    char* buffer = static_cast<char*>(data);

    while (total_received < size) {
        ssize_t received = recv(socket_fd_, buffer + total_received, size - total_received, 0);

        if (received < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted, try again
            }
            std::cerr << "[ERROR] Receive failed: " << strerror(errno) << std::endl;
            disconnect();
            return false;
        }

        if (received == 0) {
            // Client closed connection
            disconnect();
            return false;
        }

        total_received += received;
        bytes_received_ += received;
    }

    return true;
}

void ClientConnection::setAuthenticated(uint32_t user_id, const std::string& token) {
    user_id_ = user_id;
    session_token_ = token;
    authenticated_ = true;
}

void ClientConnection::disconnect() {
    if (connected_) {
        connected_ = false;
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
    }
}
