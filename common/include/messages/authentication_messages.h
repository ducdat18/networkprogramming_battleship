#ifndef AUTHENTICATION_MESSAGES_H
#define AUTHENTICATION_MESSAGES_H

#include <cstdint>
#include <cstring>

/**
 * Authentication Message Structures
 * Used for user registration, login, and session management
 */

// ============== REGISTER ==============

struct RegisterRequest {
    char username[32];      // 4-20 characters
    char password[64];      // SHA-256 hash (hex string)
    char display_name[64];  // 2-30 characters
    char email[128];        // Optional

    RegisterRequest() {
        std::memset(username, 0, sizeof(username));
        std::memset(password, 0, sizeof(password));
        std::memset(display_name, 0, sizeof(display_name));
        std::memset(email, 0, sizeof(email));
    }
} __attribute__((packed));

struct RegisterResponse {
    bool success;
    uint32_t user_id;       // 0 if failed
    char error_message[128];

    RegisterResponse()
        : success(false),
          user_id(0) {
        std::memset(error_message, 0, sizeof(error_message));
    }
} __attribute__((packed));

// ============== LOGIN ==============

struct LoginRequest {
    char username[32];
    char password[64];      // SHA-256 hash (hex string)

    LoginRequest() {
        std::memset(username, 0, sizeof(username));
        std::memset(password, 0, sizeof(password));
    }
} __attribute__((packed));

struct LoginResponse {
    bool success;
    uint32_t user_id;
    char session_token[64];
    char display_name[64];
    int32_t elo_rating;
    char error_message[128];

    LoginResponse()
        : success(false),
          user_id(0),
          elo_rating(1000) {
        std::memset(session_token, 0, sizeof(session_token));
        std::memset(display_name, 0, sizeof(display_name));
        std::memset(error_message, 0, sizeof(error_message));
    }
} __attribute__((packed));

// ============== LOGOUT ==============

struct LogoutRequest {
    char session_token[64];

    LogoutRequest() {
        std::memset(session_token, 0, sizeof(session_token));
    }
} __attribute__((packed));

struct LogoutResponse {
    bool success;

    LogoutResponse() {
        success = false;
    }
} __attribute__((packed));

// ============== SESSION VALIDATION ==============

struct SessionValidateRequest {
    char session_token[64];

    SessionValidateRequest() {
        std::memset(session_token, 0, sizeof(session_token));
    }
} __attribute__((packed));

struct SessionValidateResponse {
    bool valid;
    uint32_t user_id;
    char username[32];
    char display_name[64];
    int32_t elo_rating;
    char error_message[128];

    SessionValidateResponse()
        : valid(false),
          user_id(0),
          elo_rating(1000) {
        std::memset(username, 0, sizeof(username));
        std::memset(display_name, 0, sizeof(display_name));
        std::memset(error_message, 0, sizeof(error_message));
    }
} __attribute__((packed));

#endif // AUTHENTICATION_MESSAGES_H
