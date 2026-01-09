#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <cstdint>

// Protocol constants
#define PROTOCOL_VERSION "1.0"
#define DEFAULT_PORT 8888
#define MAX_MESSAGE_SIZE 4096
#define BUFFER_SIZE 8192

// Message types
enum MessageType {
    // Authentication
    AUTH_REGISTER = 1,
    AUTH_LOGIN = 2,
    AUTH_LOGOUT = 3,
    AUTH_RESPONSE = 4,
    VALIDATE_SESSION = 5,

    // Player management
    PLAYER_LIST_REQUEST = 10,
    PLAYER_LIST = 11,
    PLAYER_STATUS_UPDATE = 12,

    // Matchmaking
    CHALLENGE_SEND = 20,
    CHALLENGE_RECEIVED = 21,
    CHALLENGE_RESPONSE = 22,
    MATCH_START = 23,
    MATCH_READY = 24,
    QUEUE_JOIN = 25,
    QUEUE_LEAVE = 26,
    QUEUE_STATUS = 27,

    // Gameplay
    SHIP_PLACEMENT = 30,
    MOVE = 31,
    MOVE_RESULT = 32,
    TURN_UPDATE = 33,
    TURN_TIMEOUT = 34,
    MATCH_STATE = 35,
    MATCH_END = 36,

    // Match actions
    PAUSE_REQUEST = 40,
    PAUSE_RESPONSE = 41,
    DRAW_OFFER = 42,
    DRAW_RESPONSE = 43,
    RESIGN = 44,
    REMATCH_REQUEST = 45,
    REMATCH_RESPONSE = 46,

    // Replay & Stats
    REPLAY_REQUEST = 50,
    REPLAY_DATA = 51,
    STATS_REQUEST = 52,
    STATS_DATA = 53,
    ELO_UPDATE = 54,

    // Chat
    CHAT_MESSAGE = 60,

    // System
    ERROR = 99,
    NOTIFICATION = 100,
    PING = 101,
    PONG = 102
};

// Player status
enum PlayerStatus {
    STATUS_OFFLINE = 0,
    STATUS_ONLINE = 1,
    STATUS_AVAILABLE = 2,
    STATUS_IN_GAME = 3,
    STATUS_BUSY = 4
};

// Game result
enum GameResult {
    RESULT_WIN = 0,
    RESULT_LOSS = 1,
    RESULT_DRAW = 2
};

// Match end reason
enum MatchEndReason {
    END_NORMAL = 0,          // Normal game completion
    END_RESIGN = 1,          // Player resigned
    END_DISCONNECT = 2,      // Player disconnected
    END_TIMEOUT = 3,         // Player timed out
    END_DRAW_AGREED = 4      // Both players agreed to draw
};

// Shot result
enum ShotResult {
    SHOT_MISS = 0,
    SHOT_HIT = 1,
    SHOT_SUNK = 2
};

// Ship types
enum ShipType {
    SHIP_CARRIER = 0,      // 5 cells
    SHIP_BATTLESHIP = 1,   // 4 cells
    SHIP_CRUISER = 2,      // 3 cells
    SHIP_SUBMARINE = 3,    // 3 cells
    SHIP_DESTROYER = 4     // 2 cells
};

// Ship orientation
enum Orientation {
    HORIZONTAL = 0,
    VERTICAL = 1
};

// Message header structure
struct MessageHeader {
    uint8_t type;           // MessageType
    uint32_t length;        // Payload length
    uint64_t timestamp;     // Unix timestamp
    char session_token[64]; // Session token for authentication
} __attribute__((packed));

// Coordinate structure
struct Coordinate {
    int8_t row;  // 0-9
    int8_t col;  // 0-9
} __attribute__((packed));

// Ship structure
struct Ship {
    uint8_t type;           // ShipType
    uint8_t orientation;    // Orientation
    Coordinate position;    // Starting position
    uint8_t length;         // Ship length
    uint8_t hits;           // Number of hits taken
    bool is_sunk;           // Is ship sunk
} __attribute__((packed));

// Function declarations
std::string messageTypeToString(MessageType type);
const char* shipTypeToName(ShipType type);
int getShipLength(ShipType type);

#endif // PROTOCOL_H
