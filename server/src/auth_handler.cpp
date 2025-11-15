#include "auth_handler.h"
#include "message_serialization.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>

using namespace MessageSerialization;

AuthHandler::AuthHandler() : next_user_id_(1) {
    std::cout << "[AUTH] AuthHandler initialized" << std::endl;
}

AuthHandler::~AuthHandler() {
}

bool AuthHandler::canHandle(MessageType type) const {
    return type == AUTH_LOGIN ||
           type == AUTH_REGISTER ||
           type == AUTH_LOGOUT;
}

bool AuthHandler::handleMessage(ClientConnection* client,
                                const MessageHeader& header,
                                const std::string& payload) {
    std::cout << "[AUTH] Handling message type=" << (int)header.type << std::endl;

    switch (header.type) {
        case AUTH_LOGIN:
            return handleLogin(client, payload);

        case AUTH_REGISTER:
            return handleRegister(client, payload);

        case AUTH_LOGOUT:
            return handleLogout(client, payload);

        default:
            std::cerr << "[AUTH] Unknown message type: " << (int)header.type << std::endl;
            return false;
    }
}

bool AuthHandler::handleLogin(ClientConnection* client, const std::string& payload) {
    LoginRequest req;
    if (!deserialize(payload, req)) {
        std::cerr << "[AUTH] Failed to deserialize LoginRequest" << std::endl;
        return false;
    }

    std::cout << "[AUTH] Login request: username=" << req.username << std::endl;

    LoginResponse resp;

    // Phase 1.2: Simple in-memory validation
    // Phase 2: Will check against database
    {
        std::lock_guard<std::mutex> lock(users_mutex_);

        auto it = users_.find(req.username);
        if (it == users_.end()) {
            // User not found
            resp.success = false;
            safeStrCopy(resp.error_message, "User not found", sizeof(resp.error_message));
            std::cout << "[AUTH] Login failed: user not found" << std::endl;
        } else if (strcmp(it->second.password_hash.c_str(), req.password) != 0) {
            // Wrong password
            resp.success = false;
            safeStrCopy(resp.error_message, "Invalid password", sizeof(resp.error_message));
            std::cout << "[AUTH] Login failed: invalid password" << std::endl;
        } else {
            // Success!
            resp.success = true;
            resp.user_id = it->second.user_id;
            resp.elo_rating = it->second.elo_rating;
            safeStrCopy(resp.display_name, it->second.display_name, sizeof(resp.display_name));

            // Generate session token
            std::string token = generateSessionToken(resp.user_id);
            safeStrCopy(resp.session_token, token, sizeof(resp.session_token));

            // Store session
            {
                std::lock_guard<std::mutex> session_lock(sessions_mutex_);
                sessions_[token] = resp.user_id;
            }

            // Mark client as authenticated
            client->setAuthenticated(resp.user_id, token);

            std::cout << "[AUTH] Login successful: user_id=" << resp.user_id
                      << " token=" << token << std::endl;
        }
    }

    // Send response
    return sendResponse(client, AUTH_RESPONSE, serialize(resp));
}

bool AuthHandler::handleRegister(ClientConnection* client, const std::string& payload) {
    RegisterRequest req;
    if (!deserialize(payload, req)) {
        std::cerr << "[AUTH] Failed to deserialize RegisterRequest" << std::endl;
        return false;
    }

    std::cout << "[AUTH] Register request: username=" << req.username
              << " display_name=" << req.display_name << std::endl;

    RegisterResponse resp;

    // Phase 1.2: Simple in-memory storage
    // Phase 2: Will insert into database
    {
        std::lock_guard<std::mutex> lock(users_mutex_);

        // Check if username already exists
        if (users_.find(req.username) != users_.end()) {
            resp.success = false;
            safeStrCopy(resp.error_message, "Username already exists", sizeof(resp.error_message));
            std::cout << "[AUTH] Registration failed: username exists" << std::endl;
        } else {
            // Create new user
            UserData user;
            user.user_id = next_user_id_++;
            user.username = req.username;
            user.password_hash = req.password;  // Already hashed by client
            user.display_name = req.display_name;
            user.elo_rating = 1000;  // Starting ELO

            users_[req.username] = user;

            resp.success = true;
            resp.user_id = user.user_id;

            std::cout << "[AUTH] Registration successful: user_id=" << user.user_id << std::endl;
        }
    }

    // Send response
    return sendResponse(client, AUTH_RESPONSE, serialize(resp));
}

bool AuthHandler::handleLogout(ClientConnection* client, const std::string& payload) {
    LogoutRequest req;
    if (!deserialize(payload, req)) {
        std::cerr << "[AUTH] Failed to deserialize LogoutRequest" << std::endl;
        return false;
    }

    std::cout << "[AUTH] Logout request: token=" << req.session_token << std::endl;

    LogoutResponse resp;
    resp.success = true;

    // Remove session
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.erase(req.session_token);
    }

    std::cout << "[AUTH] Logout successful" << std::endl;

    // Send response
    return sendResponse(client, AUTH_RESPONSE, serialize(resp));
}

std::string AuthHandler::generateSessionToken(uint32_t user_id) {
    // Phase 1.2: Simple token generation
    // Phase 2: Use cryptographically secure random + UUID

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "token_" << user_id << "_";

    // Generate random hex string
    for (int i = 0; i < 32; i++) {
        ss << std::hex << dis(gen);
    }

    return ss.str();
}

bool AuthHandler::validateSessionToken(const std::string& token, uint32_t& user_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    auto it = sessions_.find(token);
    if (it == sessions_.end()) {
        return false;
    }

    user_id = it->second;
    return true;
}
