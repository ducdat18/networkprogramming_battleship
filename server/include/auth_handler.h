#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H

#include "message_handler.h"
#include "messages/authentication_messages.h"
#include <map>
#include <mutex>

/**
 * Authentication Handler
 * Handles LOGIN, REGISTER, LOGOUT messages
 *
 * Phase 1.2: Implements message routing and response (NO DATABASE YET)
 * Phase 2: Will add real database authentication
 */
class AuthHandler : public MessageHandler {
public:
    AuthHandler();
    ~AuthHandler() override;

    bool handleMessage(ClientConnection* client,
                      const MessageHeader& header,
                      const std::string& payload) override;

    bool canHandle(MessageType type) const override;

private:
    // Message handlers for each type
    bool handleLogin(ClientConnection* client, const std::string& payload);
    bool handleRegister(ClientConnection* client, const std::string& payload);
    bool handleLogout(ClientConnection* client, const std::string& payload);

    // Helper functions
    std::string generateSessionToken(uint32_t user_id);
    bool validateSessionToken(const std::string& token, uint32_t& user_id);

    // Temporary in-memory storage (Phase 1.2 - will be replaced with database in Phase 2)
    struct UserData {
        uint32_t user_id;
        std::string username;
        std::string password_hash;
        std::string display_name;
        int32_t elo_rating;
    };

    std::map<std::string, UserData> users_;  // username -> UserData
    std::map<std::string, uint32_t> sessions_;  // session_token -> user_id
    std::mutex users_mutex_;
    std::mutex sessions_mutex_;

    uint32_t next_user_id_;
};

#endif // AUTH_HANDLER_H
