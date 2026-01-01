#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <cstring>
#include "protocol.h"
#include "client_connection.h"

/**
 * Base class for message handlers
 * Each handler processes specific types of messages
 */
class MessageHandler {
public:
    virtual ~MessageHandler() {}

    /**
     * Handle a message from a client
     * Returns true if message was handled successfully
     */
    virtual bool handleMessage(ClientConnection* client,
                              const MessageHeader& header,
                              const std::string& payload) = 0;

    /**
     * Check if this handler can process the given message type
     */
    virtual bool canHandle(MessageType type) const = 0;

protected:
    /**
     * Send a response to the client
     */
    bool sendResponse(ClientConnection* client,
                     MessageType type,
                     const std::string& payload) {
        MessageHeader header;
        header.type = type;
        header.length = payload.size();
        header.timestamp = time(nullptr);
        memset(header.session_token, 0, sizeof(header.session_token));

        return client->sendMessage(header, payload);
    }

    /**
     * Send an error response
     */
    bool sendError(ClientConnection* /* client */, const std::string& /* error_msg */) {
        // TODO: Define ErrorMessage struct
        // For now, just log
        return true;
    }
};

#endif // MESSAGE_HANDLER_H
