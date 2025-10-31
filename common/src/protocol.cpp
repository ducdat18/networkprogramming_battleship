#include "protocol.h"

std::string messageTypeToString(MessageType type) {
    switch (type) {
        case AUTH_REGISTER: return "AUTH_REGISTER";
        case AUTH_LOGIN: return "AUTH_LOGIN";
        case AUTH_LOGOUT: return "AUTH_LOGOUT";
        case AUTH_RESPONSE: return "AUTH_RESPONSE";
        case PLAYER_LIST_REQUEST: return "PLAYER_LIST_REQUEST";
        case PLAYER_LIST: return "PLAYER_LIST";
        case CHALLENGE_SEND: return "CHALLENGE_SEND";
        case CHALLENGE_RECEIVED: return "CHALLENGE_RECEIVED";
        case CHALLENGE_RESPONSE: return "CHALLENGE_RESPONSE";
        case MATCH_START: return "MATCH_START";
        case MOVE: return "MOVE";
        case MOVE_RESULT: return "MOVE_RESULT";
        case MATCH_END: return "MATCH_END";
        case CHAT_MESSAGE: return "CHAT_MESSAGE";
        case ERROR: return "ERROR";
        case NOTIFICATION: return "NOTIFICATION";
        default: return "UNKNOWN";
    }
}

const char* shipTypeToName(ShipType type) {
    switch (type) {
        case SHIP_CARRIER: return "Aircraft Carrier";
        case SHIP_BATTLESHIP: return "Battleship";
        case SHIP_CRUISER: return "Cruiser";
        case SHIP_SUBMARINE: return "Submarine";
        case SHIP_DESTROYER: return "Destroyer";
        default: return "Unknown";
    }
}

int getShipLength(ShipType type) {
    switch (type) {
        case SHIP_CARRIER: return 5;
        case SHIP_BATTLESHIP: return 4;
        case SHIP_CRUISER: return 3;
        case SHIP_SUBMARINE: return 3;
        case SHIP_DESTROYER: return 2;
        default: return 0;
    }
}
