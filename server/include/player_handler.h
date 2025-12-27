#ifndef PLAYER_HANDLER_H
#define PLAYER_HANDLER_H

#include "message_handler.h"
#include "protocol.h"

class Server;
class PlayerManager;

/**
 * PlayerHandler - Handles player list and status messages
 *
 * Handles:
 * - PLAYER_LIST_REQUEST
 * - PLAYER_STATUS_UPDATE (handled by PlayerManager broadcast)
 */
class PlayerHandler : public MessageHandler {
public:
    PlayerHandler(Server* server, PlayerManager* player_manager);
    ~PlayerHandler() override = default;

    bool canHandle(MessageType type) const override;
    bool handleMessage(ClientConnection* client,
                      const MessageHeader& header,
                      const std::string& payload) override;

private:
    bool handlePlayerListRequest(ClientConnection* client, const std::string& payload);

    Server* server_;
    PlayerManager* player_manager_;
};

#endif // PLAYER_HANDLER_H
