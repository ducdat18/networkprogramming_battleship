#ifndef MESSAGE_SERIALIZATION_H
#define MESSAGE_SERIALIZATION_H

#include <string>
#include <cstring>
#include <cstdint>
#include "protocol.h"

/**
 * Message Serialization Helpers
 * Convert structs to/from binary strings for network transmission
 */

namespace MessageSerialization {

// ============== GENERIC SERIALIZATION ==============

/**
 * Serialize any struct to binary string
 * T must be POD (Plain Old Data) type with __attribute__((packed))
 */
template<typename T>
std::string serialize(const T& data) {
    return std::string(reinterpret_cast<const char*>(&data), sizeof(T));
}

/**
 * Deserialize binary string to struct
 * Returns false if payload size doesn't match
 */
template<typename T>
bool deserialize(const std::string& payload, T& data) {
    if (payload.size() != sizeof(T)) {
        return false;
    }
    memcpy(&data, payload.data(), sizeof(T));
    return true;
}

// ============== MESSAGE HEADER HELPERS ==============

/**
 * Create a MessageHeader with given type
 */
inline MessageHeader createHeader(MessageType type, uint32_t payload_length) {
    MessageHeader header;
    header.type = type;
    header.length = payload_length;
    header.timestamp = time(nullptr);
    memset(header.session_token, 0, sizeof(header.session_token));
    return header;
}

/**
 * Create a MessageHeader with session token
 */
inline MessageHeader createAuthenticatedHeader(MessageType type,
                                               uint32_t payload_length,
                                               const char* session_token) {
    MessageHeader header = createHeader(type, payload_length);
    if (session_token) {
        strncpy(header.session_token, session_token, sizeof(header.session_token) - 1);
    }
    return header;
}

// ============== SIZE VALIDATION ==============

/**
 * Get expected payload size for a message type
 * Returns 0 if variable size or unknown type
 */
inline size_t getExpectedPayloadSize(MessageType type) {
    // These sizes will be updated when we include message headers
    switch (type) {
        case PING:
        case PONG:
            return 0;  // No payload

        // Will be filled in as we implement each message type
        default:
            return 0;  // Unknown or variable size
    }
}

/**
 * Validate that payload size matches expected size for message type
 */
inline bool validatePayloadSize(MessageType type, size_t actual_size) {
    size_t expected = getExpectedPayloadSize(type);

    // If expected is 0, it's either no payload or variable size
    if (expected == 0) {
        return actual_size <= MAX_MESSAGE_SIZE;
    }

    return actual_size == expected;
}

// ============== HELPER FUNCTIONS ==============

/**
 * Safe string copy with null termination
 */
inline void safeStrCopy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return;

    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

/**
 * Safe string copy from std::string
 */
inline void safeStrCopy(char* dest, const std::string& src, size_t dest_size) {
    safeStrCopy(dest, src.c_str(), dest_size);
}

} // namespace MessageSerialization

#endif // MESSAGE_SERIALIZATION_H
