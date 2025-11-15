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
        memset(this, 0, sizeof(RegisterRequest));
    }
} __attribute__((packed));

struct RegisterResponse {
    bool success;
    uint32_t user_id;       // 0 if failed
    char error_message[128];

    RegisterResponse() {
        memset(this, 0, sizeof(RegisterResponse));
        success = false;
        user_id = 0;
    }
} __attribute__((packed));

// ============== LOGIN ==============

struct LoginRequest {
    char username[32];
    char password[64];      // SHA-256 hash (hex string)

    LoginRequest() {
        memset(this, 0, sizeof(LoginRequest));
    }
} __attribute__((packed));

struct LoginResponse {
    bool success;
    uint32_t user_id;
    char session_token[64];
    char display_name[64];
    int32_t elo_rating;
    char error_message[128];

    LoginResponse() {
        memset(this, 0, sizeof(LoginResponse));
        success = false;
        user_id = 0;
        elo_rating = 1000;
    }
} __attribute__((packed));

// ============== LOGOUT ==============

struct LogoutRequest {
    char session_token[64];

    LogoutRequest() {
        memset(this, 0, sizeof(LogoutRequest));
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
        memset(this, 0, sizeof(SessionValidateRequest));
    }
} __attribute__((packed));

struct SessionValidateResponse {
    bool valid;
    uint32_t user_id;
    char username[32];

    SessionValidateResponse() {
        memset(this, 0, sizeof(SessionValidateResponse));
        valid = false;
        user_id = 0;
    }
} __attribute__((packed));

#endif // AUTHENTICATION_MESSAGES_H
