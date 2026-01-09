#ifndef REPLAY_HANDLER_H
#define REPLAY_HANDLER_H

#include "message_handler.h"
#include "database.h"

class Server;

/**
 * ReplayHandler - Handles replay and match history requests
 */
class ReplayHandler : public MessageHandler {
private:
    Server* server_;
    DatabaseManager* db_;

public:
    ReplayHandler(Server* server, DatabaseManager* db);
    ~ReplayHandler();

    bool handleMessage(ClientConnection* client,
                      const MessageHeader& header,
                      const std::string& payload) override;
    bool canHandle(MessageType type) const override;

private:
    void handleReplayListRequest(const MessageHeader& header,
                                 int client_fd,
                                 uint32_t user_id);
};

#endif // REPLAY_HANDLER_H
