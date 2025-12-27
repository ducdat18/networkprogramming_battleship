#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H

#include "message_handler.h"
#include "messages/authentication_messages.h"
#include "database.h"

// Forward declarations
class Server;

/**
 * Authentication Handler
 * Handles LOGIN, REGISTER, LOGOUT messages
 *
 * Phase 2.1: Uses DatabaseManager for persistent storage
 * Phase 3.1: Registers players with PlayerManager
 */
class AuthHandler : public MessageHandler {
public:
    /**
     * Constructor
     * @param db Pointer to DatabaseManager (must be valid for lifetime of AuthHandler)
     */
    explicit AuthHandler(DatabaseManager* db);
    ~AuthHandler() override;

    // Set server reference (for PlayerManager access)
    void setServer(Server* server) { server_ = server; }

    bool handleMessage(ClientConnection* client,
                      const MessageHeader& header,
                      const std::string& payload) override;

    bool canHandle(MessageType type) const override;

private:
    // Message handlers for each type
    bool handleLogin(ClientConnection* client, const std::string& payload);
    bool handleRegister(ClientConnection* client, const std::string& payload);
    bool handleLogout(ClientConnection* client, const std::string& payload);
    bool handleValidateSession(ClientConnection* client, const std::string& payload);

    // Helper functions
    std::string generateSessionToken(uint32_t user_id);

    // Database manager (not owned by this class)
    DatabaseManager* db_;

    // Server reference (not owned, for PlayerManager access)
    Server* server_;
};

#endif // AUTH_HANDLER_H
