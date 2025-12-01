#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

/**
 * Server Configuration
 * Central place for all server configuration values
 */

// Network settings
#define SERVER_PORT 9999        // Default server port
#define MAX_CLIENTS 100         // Maximum concurrent connections
#define BUFFER_SIZE 8192        // Network buffer size

// Database settings
#define DATABASE_PATH "data/battleship.db"

// Server info
#define SERVER_VERSION "1.0"

#endif // SERVER_CONFIG_H
