#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include <ctime>

/**
 * User data structure
 */
struct User {
    uint32_t user_id;
    std::string username;
    std::string password_hash;
    std::string display_name;
    int32_t elo_rating;
    time_t created_at;
    time_t last_login;

    User() : user_id(0), elo_rating(1000), created_at(0), last_login(0) {}
};

/**
 * Session data structure
 */
struct Session {
    uint32_t session_id;
    uint32_t user_id;
    std::string session_token;
    time_t created_at;
    time_t expires_at;

    Session() : session_id(0), user_id(0), created_at(0), expires_at(0) {}

    bool isExpired() const {
        return time(nullptr) > expires_at;
    }
};

/**
 * Match data structure (for Phase 4-5)
 */
struct Match {
    uint32_t match_id;
    uint32_t player1_id;
    uint32_t player2_id;
    uint32_t winner_id;
    std::string status; // "waiting", "in_progress", "completed", "draw"
    time_t created_at;
    time_t ended_at;

    Match() : match_id(0), player1_id(0), player2_id(0), winner_id(0),
              created_at(0), ended_at(0) {}
};

/**
 * DatabaseManager - Manages SQLite database operations
 */
class DatabaseManager {
public:
    /**
     * Constructor - opens database and initializes schema
     * @param db_path Path to SQLite database file
     */
    explicit DatabaseManager(const std::string& db_path = "data/battleship.db");

    /**
     * Destructor - closes database connection
     */
    ~DatabaseManager();

    // Prevent copying
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * Check if database is open
     */
    bool isOpen() const { return db_ != nullptr; }

    // ===== USER OPERATIONS =====

    /**
     * Create a new user
     * @return user_id if success, 0 if failed
     */
    uint32_t createUser(const std::string& username,
                        const std::string& password_hash,
                        const std::string& display_name);

    /**
     * Get user by username
     * @return User object, user_id = 0 if not found
     */
    User getUserByUsername(const std::string& username);

    /**
     * Get user by user_id
     * @return User object, user_id = 0 if not found
     */
    User getUserById(uint32_t user_id);

    /**
     * Update user's last login timestamp
     */
    bool updateLastLogin(uint32_t user_id);

    /**
     * Update user's ELO rating
     */
    bool updateEloRating(uint32_t user_id, int32_t new_elo);

    /**
     * Check if username exists
     */
    bool usernameExists(const std::string& username);

    // ===== SESSION OPERATIONS =====

    /**
     * Create a new session
     * @param duration_hours Session duration in hours (default 24h)
     * @return session_id if success, 0 if failed
     */
    uint32_t createSession(uint32_t user_id,
                           const std::string& session_token,
                           int duration_hours = 24);

    /**
     * Get session by token
     * @return Session object, session_id = 0 if not found
     */
    Session getSessionByToken(const std::string& session_token);

    /**
     * Validate session (check if exists and not expired)
     * @return user_id if valid, 0 if invalid/expired
     */
    uint32_t validateSession(const std::string& session_token);

    /**
     * Delete session (logout)
     */
    bool deleteSession(const std::string& session_token);

    /**
     * Delete all expired sessions (cleanup)
     */
    int cleanupExpiredSessions();

    /**
     * Delete all sessions for a user
     */
    bool deleteUserSessions(uint32_t user_id);

    // ===== MATCH OPERATIONS (for Phase 4-5) =====

    /**
     * Create a new match
     * @return match_id if success, 0 if failed
     */
    uint32_t createMatch(uint32_t player1_id, uint32_t player2_id);

    /**
     * Get match by ID
     */
    Match getMatchById(uint32_t match_id);

    /**
     * Update match status
     */
    bool updateMatchStatus(uint32_t match_id, const std::string& status);

    /**
     * End match with winner
     */
    bool endMatch(uint32_t match_id, uint32_t winner_id);

    /**
     * Get user's match history
     */
    std::vector<Match> getUserMatches(uint32_t user_id, int limit = 10);

    // ===== BOARD OPERATIONS (for Phase 4-5) =====

    /**
     * Save player's ship placement for a match
     * @param ship_data Serialized ship placement data
     */
    bool saveShipPlacement(uint32_t match_id, uint32_t user_id,
                          const std::string& ship_data);

    /**
     * Get player's ship placement for a match
     */
    std::string getShipPlacement(uint32_t match_id, uint32_t user_id);

    // ===== MOVE OPERATIONS (for Phase 5) =====

    /**
     * Save a move in a match
     */
    bool saveMove(uint32_t match_id, uint32_t player_id,
                  int move_number, int x, int y,
                  const std::string& result);

    /**
     * Get all moves for a match
     */
    std::vector<std::string> getMatchMoves(uint32_t match_id);

    /**
     * Get last error message
     */
    std::string getLastError() const { return last_error_; }

private:
    /**
     * Initialize database schema
     */
    bool initializeSchema();

    /**
     * Execute SQL statement
     */
    bool executeSQL(const std::string& sql);

    /**
     * Prepare SQL statement
     */
    sqlite3_stmt* prepareStatement(const std::string& sql);

    sqlite3* db_;
    std::string db_path_;
    std::string last_error_;
};

#endif // DATABASE_H
