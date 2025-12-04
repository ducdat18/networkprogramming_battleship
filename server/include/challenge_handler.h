#ifndef CHALLENGE_HANDLER_H
#define CHALLENGE_HANDLER_H

#include "message_handler.h"
#include "protocol.h"

class Server;
class ChallengeManager;

/**
 * ChallengeHandler - Handles challenge-related messages
 *
 * Handles:
 * - CHALLENGE_SEND (send challenge to another player)
 * - CHALLENGE_RESPONSE (accept/decline challenge)
 */
class ChallengeHandler : public MessageHandler {
public:
    ChallengeHandler(Server* server, ChallengeManager* challenge_manager);
    ~ChallengeHandler() override = default;

    bool canHandle(MessageType type) const override;
    bool handleMessage(ClientConnection* client,
                      const MessageHeader& header,
                      const std::string& payload) override;

private:
    bool handleChallengeSend(ClientConnection* client, const std::string& payload);
    bool handleChallengeResponse(ClientConnection* client, const std::string& payload);

    Server* server_;
    ChallengeManager* challenge_manager_;
};

#endif // CHALLENGE_HANDLER_H
