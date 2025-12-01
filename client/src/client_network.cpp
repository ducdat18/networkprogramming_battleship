#include "client_network.h"
#include "message_serialization.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

using namespace MessageSerialization;

ClientNetwork::ClientNetwork()
    : socket_fd_(-1)
    , status_(DISCONNECTED)
    , port_(0)
    , user_id_(0)
    , elo_rating_(0)
    , running_(false)
    , pending_request_(NONE)
{
}

ClientNetwork::~ClientNetwork() {
    disconnect();
}

// ==================== Connection Management ====================

bool ClientNetwork::connect(const std::string& host, int port, ConnectionCallback callback) {
    if (isConnected()) {
        std::cerr << "[CLIENT] Already connected" << std::endl;
        if (callback) {
            callback(false, "Already connected");
        }
        return false;
    }

    std::cout << "[CLIENT] Connecting to " << host << ":" << port << std::endl;

    status_ = CONNECTING;
    host_ = host;
    port_ = port;
    connection_callback_ = callback;

    // Create socket connection
    if (!connectSocket(host, port)) {
        status_ = ERROR_STATE;
        if (callback) {
            callback(false, "Failed to connect to server");
        }
        return false;
    }

    status_ = CONNECTED;
    running_ = true;

    // Start receive thread
    receive_thread_ = std::thread(&ClientNetwork::receiveLoop, this);

    std::cout << "[CLIENT] Connected successfully" << std::endl;

    if (callback) {
        callback(true, "");
    }

    return true;
}

void ClientNetwork::disconnect() {
    if (!isConnected() && status_ != CONNECTING) {
        return;
    }

    std::cout << "[CLIENT] Disconnecting..." << std::endl;

    running_ = false;

    // Close socket
    closeSocket();

    // Wait for receive thread
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }

    // Reset state
    status_ = DISCONNECTED;
    user_id_ = 0;
    session_token_.clear();
    display_name_.clear();
    elo_rating_ = 0;
    pending_request_ = NONE;

    std::cout << "[CLIENT] Disconnected" << std::endl;
}

// ==================== Socket Operations ====================

bool ClientNetwork::connectSocket(const std::string& host, int port) {
    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "[CLIENT] Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = 5;  // 5 second timeout
    tv.tv_usec = 0;
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "[CLIENT] Invalid address: " << host << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[CLIENT] Connection failed: " << strerror(errno) << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    return true;
}

void ClientNetwork::closeSocket() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool ClientNetwork::sendMessage(const MessageHeader& header, const std::string& payload) {
    if (socket_fd_ < 0) {
        std::cerr << "[CLIENT] Socket not connected" << std::endl;
        return false;
    }

    // Send header
    ssize_t sent = send(socket_fd_, &header, sizeof(MessageHeader), 0);
    if (sent != sizeof(MessageHeader)) {
        std::cerr << "[CLIENT] Failed to send header" << std::endl;
        return false;
    }

    // Send payload if any
    if (header.length > 0 && !payload.empty()) {
        sent = send(socket_fd_, payload.data(), payload.size(), 0);
        if (sent != (ssize_t)payload.size()) {
            std::cerr << "[CLIENT] Failed to send payload" << std::endl;
            return false;
        }
    }

    std::cout << "[CLIENT] Sent message type=" << (int)header.type
              << " length=" << header.length << std::endl;

    return true;
}

bool ClientNetwork::receiveMessage(MessageHeader& header, std::string& payload) {
    if (socket_fd_ < 0) {
        return false;
    }

    // Receive header
    ssize_t received = recv(socket_fd_, &header, sizeof(MessageHeader), MSG_WAITALL);
    if (received != sizeof(MessageHeader)) {
        if (received == 0) {
            std::cout << "[CLIENT] Server closed connection" << std::endl;
            return false;
        } else if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout - this is normal, just no messages available
                return false;  // Will retry in receiveLoop
            } else {
                std::cerr << "[CLIENT] Receive error: " << strerror(errno) << std::endl;
                return false;
            }
        }
        return false;
    }

    // Receive payload if any
    payload.clear();
    if (header.length > 0) {
        payload.resize(header.length);
        received = recv(socket_fd_, &payload[0], header.length, MSG_WAITALL);
        if (received != (ssize_t)header.length) {
            std::cerr << "[CLIENT] Failed to receive payload" << std::endl;
            return false;
        }
    }

    std::cout << "[CLIENT] Received message type=" << (int)header.type
              << " length=" << header.length << std::endl;

    return true;
}

// ==================== Message Handling ====================

void ClientNetwork::receiveLoop() {
    std::cout << "[CLIENT] Receive loop started" << std::endl;

    while (running_) {
        MessageHeader header;
        std::string payload;

        if (!receiveMessage(header, payload)) {
            if (!running_) {
                break;  // Normal shutdown
            }
            // Check if it's just a timeout (normal) or real error
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout - no messages available, continue waiting
                continue;
            }
            // Real connection error
            std::cerr << "[CLIENT] Connection lost" << std::endl;
            status_ = ERROR_STATE;
            break;
        }

        // Handle message based on type
        MessageType msg_type = static_cast<MessageType>(header.type);

        switch (msg_type) {
            case AUTH_RESPONSE:
                handleAuthResponse(payload);
                break;

            case PONG:
                // Keepalive response
                break;

            default:
                std::cout << "[CLIENT] Unhandled message type: " << (int)msg_type << std::endl;
                break;
        }
    }

    std::cout << "[CLIENT] Receive loop stopped" << std::endl;
}

void ClientNetwork::handleAuthResponse(const std::string& payload) {
    std::lock_guard<std::mutex> lock(callback_mutex_);

    PendingRequest request = pending_request_;
    pending_request_ = NONE;

    if (request == REGISTER) {
        // Handle register response
        RegisterResponse resp;
        if (!deserialize(payload, resp)) {
            std::cerr << "[CLIENT] Failed to deserialize RegisterResponse" << std::endl;
            if (register_callback_) {
                register_callback_(false, 0, "Invalid response from server");
            }
            return;
        }

        std::cout << "[CLIENT] Register response: success=" << resp.success
                  << " user_id=" << resp.user_id << std::endl;

        if (register_callback_) {
            register_callback_(resp.success, resp.user_id,
                             resp.success ? "" : std::string(resp.error_message));
        }

    } else if (request == LOGIN) {
        // Handle login response
        LoginResponse resp;
        if (!deserialize(payload, resp)) {
            std::cerr << "[CLIENT] Failed to deserialize LoginResponse" << std::endl;
            if (login_callback_) {
                login_callback_(false, 0, "", 0, "", "Invalid response from server");
            }
            return;
        }

        std::cout << "[CLIENT] Login response: success=" << resp.success
                  << " user_id=" << resp.user_id << std::endl;

        if (resp.success) {
            // Update state
            user_id_ = resp.user_id;
            session_token_ = resp.session_token;
            display_name_ = resp.display_name;
            elo_rating_ = resp.elo_rating;
            status_ = AUTHENTICATED;
        }

        if (login_callback_) {
            login_callback_(resp.success, resp.user_id, resp.display_name,
                          resp.elo_rating, resp.session_token,
                          resp.success ? "" : std::string(resp.error_message));
        }

    } else if (request == LOGOUT) {
        // Handle logout response
        LogoutResponse resp;
        if (!deserialize(payload, resp)) {
            std::cerr << "[CLIENT] Failed to deserialize LogoutResponse" << std::endl;
            if (logout_callback_) {
                logout_callback_(false);
            }
            return;
        }

        std::cout << "[CLIENT] Logout response: success=" << resp.success << std::endl;

        if (resp.success) {
            // Clear auth state
            user_id_ = 0;
            session_token_.clear();
            display_name_.clear();
            elo_rating_ = 0;
            status_ = CONNECTED;
        }

        if (logout_callback_) {
            logout_callback_(resp.success);
        }

    } else if (request == VALIDATE_SESSION) {
        // Handle validate session response
        SessionValidateResponse resp;
        if (!deserialize(payload, resp)) {
            std::cerr << "[CLIENT] Failed to deserialize SessionValidateResponse" << std::endl;
            if (validate_session_callback_) {
                validate_session_callback_(false, 0, "", "", 0, "Invalid response from server");
            }
            return;
        }

        std::cout << "[CLIENT] Validate session response: valid=" << resp.valid
                  << " user_id=" << resp.user_id << std::endl;

        if (resp.valid) {
            // Update state
            user_id_ = resp.user_id;
            session_token_ = std::string(resp.username); // Store username temporarily
            display_name_ = resp.display_name;
            elo_rating_ = resp.elo_rating;
            status_ = AUTHENTICATED;
        }

        if (validate_session_callback_) {
            validate_session_callback_(resp.valid, resp.user_id, resp.username,
                                      resp.display_name, resp.elo_rating,
                                      resp.valid ? "" : std::string(resp.error_message));
        }
    }
}

// ==================== Authentication API ====================

void ClientNetwork::registerUser(const std::string& username,
                                 const std::string& password,
                                 const std::string& display_name,
                                 RegisterCallback callback) {
    if (!isConnected()) {
        std::cerr << "[CLIENT] Not connected to server" << std::endl;
        if (callback) {
            callback(false, 0, "Not connected to server");
        }
        return;
    }

    std::cout << "[CLIENT] Registering user: " << username << std::endl;

    // Create request
    RegisterRequest req;
    safeStrCopy(req.username, username, sizeof(req.username));
    safeStrCopy(req.password, password, sizeof(req.password));
    safeStrCopy(req.display_name, display_name, sizeof(req.display_name));

    // Set callback
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        register_callback_ = callback;
        pending_request_ = REGISTER;
    }

    // Send message
    MessageHeader header;
    header.type = static_cast<uint8_t>(AUTH_REGISTER);
    header.length = sizeof(RegisterRequest);
    header.timestamp = time(nullptr);
    memset(header.session_token, 0, sizeof(header.session_token));

    if (!sendMessage(header, serialize(req))) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        pending_request_ = NONE;
        if (callback) {
            callback(false, 0, "Failed to send request");
        }
    }
}

void ClientNetwork::loginUser(const std::string& username,
                              const std::string& password,
                              LoginCallback callback) {
    if (!isConnected()) {
        std::cerr << "[CLIENT] Not connected to server" << std::endl;
        if (callback) {
            callback(false, 0, "", 0, "", "Not connected to server");
        }
        return;
    }

    std::cout << "[CLIENT] Logging in user: " << username << std::endl;

    // Create request
    LoginRequest req;
    safeStrCopy(req.username, username, sizeof(req.username));
    safeStrCopy(req.password, password, sizeof(req.password));

    // Set callback
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        login_callback_ = callback;
        pending_request_ = LOGIN;
    }

    // Send message
    MessageHeader header;
    header.type = static_cast<uint8_t>(AUTH_LOGIN);
    header.length = sizeof(LoginRequest);
    header.timestamp = time(nullptr);
    memset(header.session_token, 0, sizeof(header.session_token));

    if (!sendMessage(header, serialize(req))) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        pending_request_ = NONE;
        if (callback) {
            callback(false, 0, "", 0, "", "Failed to send request");
        }
    }
}

void ClientNetwork::logoutUser(LogoutCallback callback) {
    if (!isAuthenticated()) {
        std::cerr << "[CLIENT] Not authenticated" << std::endl;
        if (callback) {
            callback(false);
        }
        return;
    }

    std::cout << "[CLIENT] Logging out" << std::endl;

    // Create request
    LogoutRequest req;
    safeStrCopy(req.session_token, session_token_, sizeof(req.session_token));

    // Set callback
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        logout_callback_ = callback;
        pending_request_ = LOGOUT;
    }

    // Send message
    MessageHeader header;
    header.type = static_cast<uint8_t>(AUTH_LOGOUT);
    header.length = sizeof(LogoutRequest);
    header.timestamp = time(nullptr);
    memset(header.session_token, 0, sizeof(header.session_token));

    if (!sendMessage(header, serialize(req))) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        pending_request_ = NONE;
        if (callback) {
            callback(false);
        }
    }
}

void ClientNetwork::validateSession(const std::string& session_token, ValidateSessionCallback callback) {
    if (!isConnected()) {
        std::cerr << "[CLIENT] Not connected" << std::endl;
        if (callback) {
            callback(false, 0, "", "", 0, "Not connected to server");
        }
        return;
    }

    std::cout << "[CLIENT] Validating session token" << std::endl;

    // Create request
    SessionValidateRequest req;
    safeStrCopy(req.session_token, session_token, sizeof(req.session_token));

    // Set callback
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        validate_session_callback_ = callback;
        pending_request_ = VALIDATE_SESSION;
    }

    // Send message
    MessageHeader header;
    header.type = static_cast<uint8_t>(MessageType::VALIDATE_SESSION);
    header.length = sizeof(SessionValidateRequest);
    header.timestamp = time(nullptr);
    memset(header.session_token, 0, sizeof(header.session_token));

    std::cout << "[CLIENT] DEBUG: MessageType::VALIDATE_SESSION = " << (int)MessageType::VALIDATE_SESSION << std::endl;
    std::cout << "[CLIENT] DEBUG: header.type before send = " << (int)header.type << std::endl;

    if (!sendMessage(header, serialize(req))) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        pending_request_ = NONE;
        if (callback) {
            callback(false, 0, "", "", 0, "Failed to send request");
        }
    }
}
