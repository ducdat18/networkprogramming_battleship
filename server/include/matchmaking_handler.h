#ifndef MATCHMAKING_HANDLER_H
#define MATCHMAKING_HANDLER_H

#include "message_handler.h"
#include "matchmaking_manager.h"

class Server;

/**
 * MatchmakingHandler - Handles auto-matchmaking queue messages
 */
class MatchmakingHandler : public MessageHandler {
private:
    Server* server_;
    MatchmakingManager* matchmaking_mgr_;
    DatabaseManager* db_;

public:
    MatchmakingHandler(Server* server, MatchmakingManager* mgr, DatabaseManager* db);
    ~MatchmakingHandler();

    bool handleMessage(ClientConnection* client,
                      const MessageHeader& header,
                      const std::string& payload) override;
    bool canHandle(MessageType type) const override;

private:
    void handleQueueJoin(const MessageHeader& header,
                        const QueueJoinMessage& msg,
                        int client_fd);
    void handleQueueLeave(const MessageHeader& header,
                         const QueueLeaveMessage& msg,
                         int client_fd);
};

#endif // MATCHMAKING_HANDLER_H
