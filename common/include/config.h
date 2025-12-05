#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

/**
 * Common Configuration
 * Central place for all shared configuration values between client and server
 */

// ===========================================
// Network Settings
// ===========================================
#define SERVER_HOST "127.0.0.1"  // Default server host
#define SERVER_PORT 9999         // Default server port
#define MAX_CLIENTS 100          // Maximum concurrent connections
#define BUFFER_SIZE 8192         // Network buffer size

// ===========================================
// Database Settings
// ===========================================
#define DATABASE_PATH "data/battleship.db"

// ===========================================
// Application Info
// ===========================================
#define APP_VERSION "1.0"

// ===========================================
// Client UI Settings
// ===========================================
#define WINDOW_TITLE "⚓ Battleship Game"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// ===========================================
// Auto-Login Settings
// ===========================================
// Set to 1 to enable auto-login, 0 to disable
// Disable for demo on single machine with multiple clients
#define AUTO_LOGIN_ENABLED 0

#endif // COMMON_CONFIG_H
