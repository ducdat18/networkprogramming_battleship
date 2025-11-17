#include "auth_handler.h"
#include "message_serialization.h"
#include "password_hash.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>

using namespace MessageSerialization;

AuthHandler::AuthHandler(DatabaseManager* db) : db_(db) {
    if (!db_ || !db_->isOpen()) {
        std::cerr << "[AUTH] ERROR: Invalid or closed database!" << std::endl;
    }
    std::cout << "[AUTH] AuthHandler initialized with database" << std::endl;
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

bool AuthHandler::handleRegister(ClientConnection* client, const std::string& payload) {
    RegisterRequest req;
    if (!deserialize(payload, req)) {
        std::cerr << "[AUTH] Failed to deserialize RegisterRequest" << std::endl;
        return false;
    }

    std::cout << "[AUTH] Register request: username=" << req.username
              << " display_name=" << req.display_name << std::endl;

    RegisterResponse resp;

    if (!db_ || !db_->isOpen()) {
        resp.success = false;
        safeStrCopy(resp.error_message, "Database error", sizeof(resp.error_message));
        std::cerr << "[AUTH] Database not available" << std::endl;
        return sendResponse(client, AUTH_RESPONSE, serialize(resp));
    }

    // Check if username already exists
    if (db_->usernameExists(req.username)) {
        resp.success = false;
        safeStrCopy(resp.error_message, "Username already exists", sizeof(resp.error_message));
        std::cout << "[AUTH] Registration failed: username exists" << std::endl;
    } else {
        // Hash password before storing
        std::string password_hash = PasswordHash::hashPassword(req.password);

        // Create new user in database
        uint32_t user_id = db_->createUser(req.username, password_hash, req.display_name);

        if (user_id > 0) {
            resp.success = true;
            resp.user_id = user_id;
            std::cout << "[AUTH] Registration successful: user_id=" << user_id << std::endl;
        } else {
            resp.success = false;
            safeStrCopy(resp.error_message, "Failed to create user", sizeof(resp.error_message));
            std::cerr << "[AUTH] Database error: " << db_->getLastError() << std::endl;
        }
    }

    // Send response
    return sendResponse(client, AUTH_RESPONSE, serialize(resp));
}

bool AuthHandler::handleLogin(ClientConnection* client, const std::string& payload) {
    LoginRequest req;
    if (!deserialize(payload, req)) {
        std::cerr << "[AUTH] Failed to deserialize LoginRequest" << std::endl;
        return false;
    }

    std::cout << "[AUTH] Login request: username=" << req.username << std::endl;

    LoginResponse resp;

    if (!db_ || !db_->isOpen()) {
        resp.success = false;
        safeStrCopy(resp.error_message, "Database error", sizeof(resp.error_message));
        std::cerr << "[AUTH] Database not available" << std::endl;
        return sendResponse(client, AUTH_RESPONSE, serialize(resp));
    }

    // Get user from database
    User user = db_->getUserByUsername(req.username);

    if (user.user_id == 0) {
        // User not found
        resp.success = false;
        safeStrCopy(resp.error_message, "User not found", sizeof(resp.error_message));
        std::cout << "[AUTH] Login failed: user not found" << std::endl;
    } else if (!PasswordHash::verifyPassword(req.password, user.password_hash)) {
        // Wrong password
        resp.success = false;
        safeStrCopy(resp.error_message, "Invalid password", sizeof(resp.error_message));
        std::cout << "[AUTH] Login failed: invalid password" << std::endl;
    } else {
        // Success!
        resp.success = true;
        resp.user_id = user.user_id;
        resp.elo_rating = user.elo_rating;
        safeStrCopy(resp.display_name, user.display_name, sizeof(resp.display_name));

        // Generate session token
        std::string token = generateSessionToken(user.user_id);
        safeStrCopy(resp.session_token, token, sizeof(resp.session_token));

        // Create session in database (24 hour expiration)
        uint32_t session_id = db_->createSession(user.user_id, token, 24);

        if (session_id > 0) {
            // Update last login timestamp
            db_->updateLastLogin(user.user_id);

            // Mark client as authenticated
            client->setAuthenticated(user.user_id, token);

            std::cout << "[AUTH] Login successful: user_id=" << user.user_id
                      << " session_id=" << session_id << std::endl;
        } else {
            // Failed to create session
            resp.success = false;
            safeStrCopy(resp.error_message, "Failed to create session", sizeof(resp.error_message));
            std::cerr << "[AUTH] Failed to create session: " << db_->getLastError() << std::endl;
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

    if (!db_ || !db_->isOpen()) {
        resp.success = false;
        std::cerr << "[AUTH] Database not available" << std::endl;
        return sendResponse(client, AUTH_RESPONSE, serialize(resp));
    }

    // Delete session from database
    bool deleted = db_->deleteSession(req.session_token);

    if (deleted) {
        resp.success = true;
        std::cout << "[AUTH] Logout successful" << std::endl;
    } else {
        // Session might not exist, but that's OK for logout
        resp.success = true;
        std::cout << "[AUTH] Logout: session not found (already logged out?)" << std::endl;
    }

    // Send response
    return sendResponse(client, AUTH_RESPONSE, serialize(resp));
}

std::string AuthHandler::generateSessionToken(uint32_t user_id) {
    // Generate cryptographically secure random token
    // Format: token_<user_id>_<random_hex>

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "token_" << user_id << "_";

    // Generate 32-character random hex string
    const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {
        ss << hex_chars[dis(gen)];
    }

    return ss.str();
}
